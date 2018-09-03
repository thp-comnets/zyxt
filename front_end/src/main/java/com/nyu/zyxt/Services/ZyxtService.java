package com.nyu.zyxt.Services;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ArrayNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.opencsv.CSVReader;
import com.opencsv.CSVWriter;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.http.ResponseEntity;
import org.springframework.stereotype.Service;
import org.springframework.util.FileSystemUtils;
import org.springframework.web.client.RestTemplate;

import javax.annotation.PostConstruct;
import javax.servlet.http.HttpServletRequest;
import java.io.*;
import java.lang.reflect.Field;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.text.DecimalFormat;
import java.util.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

@Service
public class ZyxtService {

    private BlockingQueue<ZyxtJobHandler> queue;
    private Map<String, ZyxtJobHandler> jobHandlerMap;
    @Value("${mainOutputDir}")
    private String mainOutputDir;
    @Value("${mainZyxtDir}")
    private String mainZyxtDir;
    private Map<Integer, Equipment> allEquipmentsMap;
    private Logger logger = LoggerFactory.getLogger(this.getClass());

    @Autowired
    private ZyxtJobRunner zyxtJobRunner;

    @PostConstruct
    public void init() throws IOException {
        this.jobHandlerMap = new HashMap<>();
        this.allEquipmentsMap = new HashMap<>();
        logger.info(mainOutputDir + "..." + mainZyxtDir);
        this.initializeEquipmentsMap();
        this.queue = new LinkedBlockingQueue<>();
        zyxtJobRunner.setQueue(queue);
        zyxtJobRunner.run();
        logger.info("Job Runner Started In Another Thread....");
    }

    public Map<String, ZyxtJobHandler> getJobHandlerMap() {
        return jobHandlerMap;
    }

    public ZyxtJobHandler handelZyxtInputJob(List<ZyxtInput> zyxtInputList, List<ZyxtEdges> zyxtEdgesList, List<Equipment> selectedEquipments, int mountingHeight, int tradeOffValue) throws IOException, InterruptedException {
        ZyxtJobHandler zyxtJobHandler = new ZyxtJobHandler(zyxtInputList, zyxtEdgesList, selectedEquipments, mountingHeight, tradeOffValue, this.mainZyxtDir);
        zyxtJobHandler.createInput();
        if(zyxtJobHandler.isStatus()){
            MapProcessing mapProcessing = new MapProcessing(zyxtInputList);
            mapProcessing.processMap();
            zyxtJobHandler.setRemovalPercentage(mapProcessing.getExpectedRows().intValue() * mapProcessing.getExpectedCols().intValue());
            zyxtJobHandler.createAndHandelCommand();
            this.jobHandlerMap.put(zyxtJobHandler.getJobId(), zyxtJobHandler);
            this.queue.put(zyxtJobHandler);
            logger.info("Job handled Successfully");
        }
        return zyxtJobHandler;
    }


    public String checkValidityOfJob(String jobId){
        String jobDir = this.mainOutputDir + "/newInputMap_"+jobId;
        if(Files.exists(Paths.get(jobDir)) && Files.isDirectory(Paths.get(jobDir))){
            if(!new File(   jobDir + "/acknowledged.txt").exists()){
                return "We are still processing your map.";
            }
        }else{
            return "No Job Exist For The Job Id: " + jobId;
        }
        return "TRUE";
    }

    public String fetchInputOfJobs(String jobId) throws IOException {
        String jobInputPath = this.mainZyxtDir + "/customerNodes/" + jobId;
        BufferedReader bufferedReader;
        bufferedReader = new BufferedReader(new FileReader(new File(jobInputPath + "/newInputMap_"+jobId+".csv")));
        String line;
        ObjectMapper objectMapper = new ObjectMapper();
        ArrayNode arrayNode = objectMapper.createArrayNode();
        while ((line = bufferedReader.readLine()) != null) {
            ObjectNode objectNode = objectMapper.createObjectNode();
            objectNode.put("lat", Double.parseDouble(line.split(",")[0]));
            objectNode.put("lng", Double.parseDouble(line.split(",")[1]));
            objectNode.put("type", "sink");
            objectNode.put("mountingHeight", Integer.parseInt(line.split(",")[4]));
            objectNode.put("capacity", Integer.parseInt(line.split(",")[2]));
            arrayNode.add(objectNode);
        }
        bufferedReader.close();
        //reading source
        bufferedReader = new BufferedReader(new FileReader(new File(jobInputPath + "/newInputMapSource_"+jobId+".csv")));
        while ((line = bufferedReader.readLine()) != null) {
            ObjectNode objectNode = objectMapper.createObjectNode();
            objectNode.put("lat", Double.parseDouble(line.split(",")[0]));
            objectNode.put("lng", Double.parseDouble(line.split(",")[1]));
            objectNode.put("mountingHeight", Integer.parseInt(line.split(",")[2]));
            objectNode.put("type", "source");
            arrayNode.add(objectNode);
        }
        bufferedReader.close();
        //reading lines
        bufferedReader = new BufferedReader(new FileReader(new File(jobInputPath + "/newInputMapEdges_"+jobId+".csv")));
        while ((line = bufferedReader.readLine()) != null) {
            ObjectNode objectNode = objectMapper.createObjectNode();
            objectNode.put("lat1", Double.parseDouble(line.split(",")[0]));
            objectNode.put("lng1", Double.parseDouble(line.split(",")[1]));
            objectNode.put("lat2", Double.parseDouble(line.split(",")[2]));
            objectNode.put("lng2", Double.parseDouble(line.split(",")[3]));
            objectNode.put("type", "line");
            arrayNode.add(objectNode);
        }
        bufferedReader.close();

        logger.info(arrayNode.toString());
        return arrayNode.toString();
    }

    public boolean cancelAndDeleteJob(String job_id, String user_id) throws NoSuchFieldException, IllegalAccessException, IOException {
        ZyxtJobHandler zyxtJobHandler = this.jobHandlerMap.get(job_id);
        Field f = zyxtJobHandler.getJobProcess().getClass().getDeclaredField("pid");
        f.setAccessible(true);
        long pid = f.getLong(zyxtJobHandler.getJobProcess());
        f.setAccessible(false);
        logger.info("PID: " + pid);
        Process cp = Runtime.getRuntime().exec("pgrep -P " + pid);
        long childPid = Long.parseLong(new BufferedReader(new InputStreamReader(cp.getInputStream())).readLine());
        logger.info("Child PID: " + childPid);
        Runtime.getRuntime().exec("kill " + (childPid) );
        Runtime.getRuntime().exec("kill " + (childPid+1) );
        Runtime.getRuntime().exec("kill " + pid);
        return this.deleteJob(user_id, job_id);
    }

    public int getDefaultMountingHeightOfJob(String jobId){
        ZyxtJobHandler zyxtJobHandler = this.jobHandlerMap.get(jobId);
        if(zyxtJobHandler != null){
            return zyxtJobHandler.getMountingHeight();
        }
        return 10;
    }

    private void initializeEquipmentsMap() throws IOException {
        BufferedReader bufferedReader = new BufferedReader(new FileReader(new File(this.mainZyxtDir + "/AllEquipments.csv")));
        String line;
        while ((line = bufferedReader.readLine()) != null) {
            logger.info(line);
            Equipment equipment = new Equipment();
            equipment.setId(Integer.parseInt(line.split(",")[0]));
            equipment.setName(line.split(",")[1]);
            equipment.setThroughput(Double.parseDouble(line.split(",")[2]));
            equipment.setRange(Double.parseDouble(line.split(",")[3]));
            equipment.setCost(Double.parseDouble(line.split(",")[4]));
            this.allEquipmentsMap.put(equipment.getId(), equipment);
        }
    }

    public List<Equipment> getAllEquipments(String jobId, String result) throws IOException {
        List<Equipment> allEquipments = new ArrayList<>();
        this.allEquipmentsMap.forEach((equipmentId, equipment) ->  {
            equipment.setSelected(true);
            allEquipments.add(equipment);
        });
        if(jobId != null && result.equals("TRUE")){
            allEquipments.clear();
            String jobEquipments = this.mainZyxtDir + "/customerNodes/" + jobId + "/newInputMapEquipments_"+jobId+".csv";
            Map<Integer, Equipment> equipmentMap = Equipment.readEquipmentsFromFile(jobEquipments);
            equipmentMap.forEach((equipmentId, equipment) -> allEquipments.add(equipment));
            for(Equipment eq : allEquipments){
                if(!isEquipmentSelectedInJob(eq.getId(), jobId)){
                    eq.setSelected(false);
                }
            }
        }

        return allEquipments;
    }

    private boolean isEquipmentSelectedInJob(int equipmentId, String jobId) throws IOException {
        boolean res = false;
        String jobDir = this.mainOutputDir + "/newInputMap_"+jobId;
        if(!new File(jobDir + "/MCFOutput.txt").exists()){
            res = true;
        }else{
            BufferedReader bufferedReader = new BufferedReader(new FileReader(new File(jobDir + "/MCFOutput.txt")));
            List<String> allLines = new ArrayList<>();
            String line;
            while ((line = bufferedReader.readLine()) != null) {
                allLines.add(line);
            }
            bufferedReader.close();
            File outputFile = returnFinalOutputJsonFile(allLines.get(2));
            ObjectMapper objectMapper = new ObjectMapper();
            String content = new String ( Files.readAllBytes(outputFile.toPath()) );
            JsonNode jsonNode = objectMapper.readValue(content, JsonNode.class);
            ArrayNode allEdges = (ArrayNode) jsonNode.get("edges");
            Set<Integer> idsUsedInJob = new HashSet<>();
            for(int i = 0; i < allEdges.size(); i++){
                JsonNode edgeProp = allEdges.get(i);
                idsUsedInJob.add(edgeProp.get("edgeProperty").get("deviceId").asInt());
            }
            for(int id : idsUsedInJob){
                if(id == equipmentId){
                    res = true;
                    break;
                }
            }
        }
        return res;
    }


    public List<Job> getAllJobsWithStatus(List<String> allJobs) throws IOException {
        List<Job> allJobsWithStatus = new ArrayList<>();
        if(allJobs != null){
            for (String job : allJobs) {
                Job job1 = new Job();
                String jobDir = this.mainOutputDir + "/newInputMap_" + job;
                job1.setJobId(job);
                job1.setTime(" - ");
                job1.setAction("Cancel");
                job1.setStatus("warning");
                if (Files.exists(Paths.get(jobDir)) && Files.isDirectory(Paths.get(jobDir))) {
                    if (new File(jobDir + "/acknowledged.txt").exists()) {
                        List<String> allLines = getLinesOfOutputFile(job);
                        Double timeTakes = Double.parseDouble(allLines.get(4).split(" ")[2]) / 60;
                        DecimalFormat df = new DecimalFormat("#.#");
                        job1.setTime(df.format(timeTakes)+" m");
                        job1.setStatus("success");
                        job1.setAction("Delete");
                        Job.parseAndSetCostsOfJob(job1, this.getJsonNodeOfFinalOutputJson(job), this.mainZyxtDir);
                    }
                }
                allJobsWithStatus.add(job1);
            }

        }
        return allJobsWithStatus;
    }



    public boolean deleteJob(String userId,String jobId) throws IOException {
        File userInputFile = new File(this.mainZyxtDir + "/all_users.csv");
        CSVReader csvReader = new CSVReader(new FileReader(userInputFile));
        List<String[]> csvBody = csvReader.readAll();
        for ( String[] row : csvBody) {
            if (row[0].equals(userId)) {
                List<String> allJobs = new LinkedList<>(Arrays.asList(row[2].split(",")));
                for (int i = 0; i < allJobs.size(); i++) {
                    String cJob = allJobs.get(i);
                   if(cJob.equals(jobId)){
                       allJobs.remove(i);
                   }
                }
                row[2] = String.join(",", allJobs);
                break;
            }
        }
        this.updateUserFile(csvBody, userInputFile);

        String jobDir = this.mainOutputDir + "/newInputMap_"+jobId;
        this.jobHandlerMap.remove(jobId);
        String inputDir = this.mainZyxtDir + "/customerNodes/"+jobId;
        return FileSystemUtils.deleteRecursively(new File(jobDir)) && FileSystemUtils.deleteRecursively(new File(inputDir));
//        return true;
    }

    public String processOutput(String jobId) throws IOException {
        List<String> allLines = getLinesOfOutputFile(jobId);
        if(allLines == null){
            return "FALSE";
        }
        for(String l : allLines){
            logger.info(l);
        }
        logger.info("*****************");
        logger.info(allLines.get(2));
        File outputFile = returnFinalOutputJsonFile(allLines.get(2));

        String content = new String ( Files.readAllBytes(outputFile.toPath()) );
        ObjectMapper objectMapper = new ObjectMapper();
        JsonNode jsonNode = objectMapper.readValue(content, JsonNode.class);
        return jsonNode.toString();
    }

    private JsonNode getJsonNodeOfFinalOutputJson(String job_id) throws IOException {
        List<String> allLines = getLinesOfOutputFile(job_id);
        if(allLines == null){
            return null;
        }
        for(String l : allLines){
            logger.info(l);
        }
        logger.info("*****************");
        logger.info(allLines.get(2));
        File outputFile = returnFinalOutputJsonFile(allLines.get(2));

        String content = new String ( Files.readAllBytes(outputFile.toPath()) );
        ObjectMapper objectMapper = new ObjectMapper();
        return objectMapper.readValue(content, JsonNode.class);
    }

    private List<String> getLinesOfOutputFile(String jobId) throws IOException {
        String jobDir = this.mainOutputDir + "/newInputMap_"+jobId;
        if(!new File(jobDir + "/MCFOutput.txt").exists()){
            return null;
        }
        BufferedReader bufferedReader = new BufferedReader(new FileReader(new File(jobDir + "/MCFOutput.txt")));
        List<String> allLines = new ArrayList<>();
        String line;
        while ((line = bufferedReader.readLine()) != null) {
            allLines.add(line);
        }
        bufferedReader.close();
        return allLines;
    }

    private File returnFinalOutputJsonFile(String outputPath){
        File outputFile;
        if(new File(outputPath + "/final_2_2.json").exists()){
            outputFile = new File(outputPath + "/final_2_2.json");
        }else if(new File(outputPath + "/final_1_2.json").exists()){
            outputFile = new File(outputPath + "/final_1_2.json");
        }else{
            int startIter = 0;
            boolean checkFile;
            do{
                checkFile = false;
                if(new File(outputPath + "/output_"+Integer.toString(startIter)+".json").exists()){
                    checkFile = true;
                    startIter++;
                }
            }while(checkFile);
            startIter--;
            outputFile = new File(outputPath + "/output_"+Integer.toString(startIter)+".json");
        }
        //read json output
//        logger.info("Final Output: " + outputFile.toPath());
        return outputFile;
    }

    public void insertNewJob(String userId, String newJobId) throws IOException {
        File userInputFile = new File(this.mainZyxtDir + "/all_users.csv");
        CSVReader csvReader = new CSVReader(new FileReader(userInputFile));
        List<String[]> csvBody = csvReader.readAll();
        for (String[] row : csvBody) {
            if (row[0].equals(userId)) {
                if (row[2].equals("")) {
                    row[2] = newJobId;
                } else {
                    row[2] += "," + newJobId;
                }
                break;
            }
        }
        csvReader.close();
        this.updateUserFile(csvBody, userInputFile);
    }

    public List<String> getAllJobs(String userId) throws IOException {
        File userInputFile = new File(this.mainZyxtDir + "/all_users.csv");
        CSVReader csvReader = new CSVReader(new FileReader(userInputFile));
        List<String[]> csvBody = csvReader.readAll();
        List<String> allJobs = new ArrayList<>();
        for ( String[] row : csvBody) {
           if(row[0].equals(userId) && !row[2].equals("")){
               allJobs = Arrays.asList(row[2].split(","));
           }
        }
        return allJobs;
    }

    public void saveNewUser(String userId, HttpServletRequest request) throws IOException {
        String ip_address = "";
        ObjectMapper objectMapper = new ObjectMapper();
        if (request != null) {
            ip_address = request.getHeader("X-FORWARDED-FOR");
            if (ip_address == null || "".equals(ip_address)) {
                ip_address = request.getRemoteAddr();
            }
//            ip_address = "91.230.41.202";
        }
        RestTemplate restTemplate = new RestTemplate();
        String resourceUrl = "http://api.ipstack.com/"+ip_address+"?access_key=32d455e835b3f69dd6bcc4b69efc58ab&format=1";
        ResponseEntity<String> response = restTemplate.getForEntity(resourceUrl, String.class);
        ObjectNode objectNode = objectMapper.readValue(response.getBody(), ObjectNode.class);
        File userInputFile = new File(this.mainZyxtDir + "/all_users.csv");
        CSVReader csvReader = new CSVReader(new FileReader(userInputFile));
        List<String[]> csvBody = csvReader.readAll();
        logger.info(objectNode.get("location").get("country_flag").toString());
        String [] newRow = {userId, ip_address, "", objectNode.get("city").toString().substring(1, objectNode.get("city").toString().length()-1), objectNode.get("country_name").toString().substring(1, objectNode.get("country_name").toString().length()-1), objectNode.get("location").get("country_flag").toString().substring(1, objectNode.get("location").get("country_flag").toString().length()-1)};
        csvBody.add(newRow);
        this.updateUserFile(csvBody, userInputFile);
        csvReader.close();
    }

    private void updateUserFile(List<String[]> csvBody, File userInputFile) throws IOException {
        FileWriter fileWriter = new FileWriter(userInputFile);
        CSVWriter writer = new CSVWriter(fileWriter);
        writer.writeAll(csvBody);
        writer.flush();
        writer.close();

    }

    public List<String[]> getAllUsers() throws IOException {
        File userInputFile = new File(this.mainZyxtDir + "/all_users.csv");
        CSVReader csvReader = new CSVReader(new FileReader(userInputFile));
        List<String[]> csvBody = csvReader.readAll();
        for (String[] aCsvBody : csvBody) {
            List<String> allJobs = (!aCsvBody[2].equals("")) ? Arrays.asList(aCsvBody[2].split(",")) : new ArrayList<>();
            for (int j = 0; j < allJobs.size(); j++) {
                String isValid = this.checkValidityOfJob(allJobs.get(j));
                if (isValid.equals("TRUE")) {
                    allJobs.set(j, allJobs.get(j) + ":1");
                } else {
                    allJobs.set(j, allJobs.get(j) + ":0");
                }
            }
            aCsvBody[2] = String.join(",", allJobs);
        }
        return csvBody;
    }

}
