package com.nyu.zyxt.Services;

import org.apache.commons.lang3.RandomStringUtils;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.BufferedWriter;
import java.io.FileWriter;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.List;

public class ZyxtJobHandler {

    private String jobId;
    private String command;
    private int rescalingInput;
    private boolean status;
    private int mountingHeight;
    private String zyxtDir;
    private double removalPercentage;
    private int tradeOffValue;
    private Process jobProcess;
    private List<ZyxtInput> zyxtInputList;
    private List<ZyxtEdges> zyxtEdgesList;
    private List<Equipment> selectedEquipments;
    private Logger logger = LoggerFactory.getLogger(this.getClass());

    ZyxtJobHandler(List<ZyxtInput> zyxtInputs, List<ZyxtEdges> zyxtEdgesList, List<Equipment> selectedEquipments, int mHeight, int tradeOffVal, String zyxtDir) {
        this.zyxtInputList = zyxtInputs;
        this.mountingHeight = mHeight;
        this.tradeOffValue = tradeOffVal;
        this.zyxtEdgesList = zyxtEdgesList;
        this.zyxtDir = zyxtDir;
        this.selectedEquipments = selectedEquipments;
    }

    public int getMountingHeight() {
        return mountingHeight;
    }

    public String getCommand() {
        return command;
    }

    public Process getJobProcess() {
        return jobProcess;
    }

    public void setJobProcess(Process jobProcess) {
        this.jobProcess = jobProcess;
    }


    void setRemovalPercentage(int expectedResolution){
        if (expectedResolution > 640000){
            this.rescalingInput = 638000;
            if(this.tradeOffValue == 0){
                this.tradeOffValue = 1;
            }
            this.removalPercentage = 99.7;
        }else if(expectedResolution >= 360000){
            this.removalPercentage = 99.5;
        }else if(expectedResolution >= 160000){
            this.removalPercentage = 99.3;
        }else if(expectedResolution >= 40000){
            this.removalPercentage = 99.0;
        }else if(expectedResolution >= 10000){
            this.removalPercentage = 98.5;
        }
    }

    public boolean isStatus() {
        return status;
    }

    public String getJobId() {
        return jobId;
    }


    void createAndHandelCommand() {
        switch (this.tradeOffValue){
            case 0:
                this.command = "python "+zyxtDir+"/processMapWithOriginalResolution.py " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMap_"+this.jobId+".csv " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapSource_"+this.jobId+".csv c "+Double.toString(this.removalPercentage)+" mcf " + this.zyxtDir + " " + this.mountingHeight + " " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEdges_"+this.jobId+".csv " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEquipments_"+this.jobId+".csv";
                this.status = true;
                logger.info(command);
                break;
            case 1:
                this.command = "python "+zyxtDir+"/processMapsWithProportionalReScaling.py " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMap_"+this.jobId+".csv " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapSource_"+this.jobId+".csv "+Integer.toString(this.rescalingInput)+" c "+Double.toString(this.removalPercentage)+" mcf " + this.zyxtDir + " " + this.mountingHeight + " " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEdges_"+this.jobId+".csv " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEquipments_"+this.jobId+".csv";
                this.status = true;
                logger.info(command);
                break;
            case 100:
                this.rescalingInput = 1600;
                this.removalPercentage = 0;
                this.command = "python "+zyxtDir+"/processMapsWithProportionalReScaling.py " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMap_"+this.jobId+".csv " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapSource_"+this.jobId+".csv "+Integer.toString(this.rescalingInput)+" n "+Double.toString(this.removalPercentage)+" mcf " + this.zyxtDir + " " + this.mountingHeight + " " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEdges_"+this.jobId+".csv " + zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEquipments_"+this.jobId+".csv";
                this.status = true;
                logger.info(command);
                break;
        }

    }

    void createInput() throws IOException {
        if(this.zyxtInputList.size() == 0){
            this.status = false;
            return;
        }
        this.jobId = RandomStringUtils.randomAlphanumeric(6);
        if(!Files.exists(Paths.get(zyxtDir + "/customerNodes/"+this.jobId))){
            Files.createDirectories(Paths.get(zyxtDir + "/customerNodes/"+this.jobId));
        }
        BufferedWriter sinkWriter = new BufferedWriter(new FileWriter(zyxtDir + "/customerNodes/"+this.jobId+"/newInputMap_"+this.jobId+".csv"));
        BufferedWriter sourceWriter = new BufferedWriter(new FileWriter(zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapSource_"+jobId+".csv"));
        BufferedWriter edgesWriter = new BufferedWriter(new FileWriter(zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEdges_"+jobId+".csv"));
        BufferedWriter equipmentsWriter = new BufferedWriter(new FileWriter(zyxtDir + "/customerNodes/"+this.jobId+"/newInputMapEquipments_"+jobId+".csv"));
        StringBuilder sinkContent = new StringBuilder();
        StringBuilder sourceContent = new StringBuilder();
        StringBuilder edgesContent = new StringBuilder();
        StringBuilder equipmentsContent = new StringBuilder();
        for (ZyxtInput zyxtInput : zyxtInputList) {
            if (zyxtInput.getType().equals("sink")) {
                sinkContent.append(Double.toString(zyxtInput.getLat())).append(",").append(Double.toString(zyxtInput.getLng())).append(",").append(Integer.toString(zyxtInput.getCapacity())).append(",1500").append(",").append(zyxtInput.getHeight()).append("\n");
            }
        }

        for (ZyxtInput zyxtInput : zyxtInputList) {
            if (zyxtInput.getType().equals("source")) {
                sinkContent.append(Double.toString(zyxtInput.getLat())).append(",").append(Double.toString(zyxtInput.getLng())).append(",").append(Integer.toString(zyxtInput.getCapacity())).append(",1500").append(",").append(zyxtInput.getHeight()).append("\n");
                sourceContent.append(Double.toString(zyxtInput.getLat())).append(",").append(Double.toString(zyxtInput.getLng())).append(",").append(zyxtInput.getHeight());
            }
        }

        for(ZyxtEdges zyxtEdge : this.zyxtEdgesList){
            edgesContent.append(Double.toString(zyxtEdge.getLat1())).append(",").append(Double.toString(zyxtEdge.getLng1())).append(",").
                    append(Double.toString(zyxtEdge.getLat2())).append(",").append(Double.toString(zyxtEdge.getLng2())).append("\n");
        }

        for(Equipment e : selectedEquipments){
            equipmentsContent.append(e.getName()).append(",").append(Double.toString(e.getThroughput())).append(",").
                    append(Double.toString(e.getRange())).append(",").append(Double.toString(e.getCost())).append(",").
                    append(Integer.toString(e.getId())).append("\n");
        }

        sinkWriter.write(sinkContent.toString());
        sourceWriter.write(sourceContent.toString());
        edgesWriter.write(edgesContent.toString());
        equipmentsWriter.write(equipmentsContent.toString());
        sourceWriter.close();
        sinkWriter.close();
        edgesWriter.close();
        equipmentsWriter.close();
        this.status = true;
    }

}
