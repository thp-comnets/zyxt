package com.nyu.zyxt.Controllers;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.nyu.zyxt.Services.*;
import com.opencsv.CSVReader;
import org.apache.commons.io.IOUtils;
import org.apache.commons.lang3.RandomStringUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.MediaType;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.*;
import javax.servlet.ServletContext;
import javax.servlet.http.Cookie;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import java.io.*;
import java.util.List;

@Controller
public class HomeController {

    @Autowired
    ZyxtService zyxtService;
    @Autowired
    ServletContext servletContext;
    private Logger logger = LoggerFactory.getLogger(this.getClass());

    @GetMapping(value = "/")
    public String home(Model model, @RequestParam(value = "job_id", required = false)String job_id,
                       @CookieValue(value = "job_id", required = false) String cookieJobId,
                       @CookieValue(value = "user_id", required = false) String userId,
                       HttpServletResponse response, HttpServletRequest request
                       ) throws IOException {
        logger.info(userId);
        if(job_id != null){
            String result = zyxtService.checkValidityOfJob(job_id);
            if (result.equals("TRUE")){
                String jobOutput = zyxtService.processOutput(job_id);
                model.addAttribute("default_mounting_height", zyxtService.getDefaultMountingHeightOfJob(job_id));
                model.addAttribute("output", jobOutput);
                //return "output";
            }
            model.addAttribute("result", result);
        }
        if(userId != null){
            List<String> allJobs = zyxtService.getAllJobs(userId);//Arrays.asList(new String(Base64.getDecoder().decode(cookieJobId)).split(","));
            model.addAttribute("all_jobs", zyxtService.checkStatusesOfAllJobs(allJobs));
        }
        if(userId == null){
            String newUserId = RandomStringUtils.randomNumeric(6);
            Cookie cookie = new Cookie("user_id", newUserId);
            cookie.setMaxAge(31556952);
            response.addCookie(cookie);
            zyxtService.saveNewUser(newUserId, request);
        }
        List<Equipment> allEquipments = zyxtService.getAllEquipments(job_id);
        model.addAttribute("allEquipments", allEquipments);
        return "home";
    }

    @PostMapping(value = "/submit_job")
    @ResponseBody
    public String submitJob(@RequestParam(value = "data")String data,
                            @RequestParam(value = "data_edges")String dataEdges,
                            @RequestParam(value = "data_equipments")String dataEquipments,
                            @RequestParam(value = "trade_off")Integer tradeOffValue,
                            @RequestParam(value = "mounting_height")Integer mountingHeight,
                            @CookieValue(value = "user_id") String cookieUserId) throws IOException, InterruptedException {
        ObjectMapper objectMapper = new ObjectMapper();
        ObjectNode objectNode = objectMapper.createObjectNode();
        List<String> allJobs = zyxtService.getAllJobs(cookieUserId);//Arrays.asList(new String(Base64.getDecoder().decode(cookieJobId)).split(","));
        if(allJobs.size() == 3){
            objectNode.put("status", false);
            objectNode.put("errorMessage", "Only 3 Jobs Allowed");
            return objectNode.toString();
        }
        List<ZyxtInput> zyxtInputList = objectMapper.readValue(data, new TypeReference<List<ZyxtInput>>(){});
        List<ZyxtEdges> zyxtEdgesList = objectMapper.readValue(dataEdges, new TypeReference<List<ZyxtEdges>>(){});
        List<Integer> selectedEquipments = (List<Integer>) objectMapper.readValue(dataEquipments, List.class);

        ZyxtJobHandler zyxtJobHandler = zyxtService.handelZyxtInputJob(zyxtInputList, zyxtEdgesList, selectedEquipments, mountingHeight, tradeOffValue);
//        String allJobIds;
//        if(!cookieJobId.equals("")){
//            allJobIds = new String(Base64.getDecoder().decode(cookieJobId));
//            allJobIds += "," + zyxtJobHandler.getJobId();
//        }else{
//            allJobIds = zyxtJobHandler.getJobId();
//        }
//        logger.info("Final Cookie Value: " + allJobIds);
//        String encoded = Base64.getEncoder().encodeToString(allJobIds.getBytes());
//        logger.info("Encoded String: " + encoded);
//        Cookie cookie = new Cookie("job_id", encoded);
//        cookie.setMaxAge(31556952);
//        response.addCookie(cookie);
        if(zyxtJobHandler.isStatus()){
            zyxtService.insertNewJob(cookieUserId, zyxtJobHandler.getJobId());
        }
        objectNode.put("job_id", zyxtJobHandler.getJobId());
        objectNode.put("status", zyxtJobHandler.isStatus());
        objectNode.put("errorMessage", "Something wrong with the input");
        return objectNode.toString();
    }

    @GetMapping(value = "/test_output")
    @ResponseBody
    public String testOutput(@RequestParam(value = "job_id")String jobId) throws IOException {
        zyxtService.processOutput(jobId);
        return "";
    }

    @PostMapping(value = "/delete_job/{job_id}/{user_id}")
    @ResponseBody
    public boolean deleteJob(@PathVariable String job_id, @PathVariable String user_id) throws IOException {
        return this.zyxtService.deleteJob(user_id, job_id);
    }

    @PostMapping(value = "/cancel_job/{job_id}/{user_id}")
    @ResponseBody
    public boolean cancelJob(@PathVariable String job_id,  @PathVariable String user_id) throws NoSuchFieldException, IllegalAccessException, IOException {
        return this.zyxtService.cancelAndDeleteJob(job_id, user_id);
//        return true;

    }

    @GetMapping(value = "/get_img")
    public void getFile(HttpServletResponse response) throws IOException {
        InputStream in =  new FileInputStream("/home/cuda/Downloads/zplan1.png");
        response.setContentType(MediaType.IMAGE_PNG_VALUE);
        IOUtils.copy(in, response.getOutputStream());
    }

}
