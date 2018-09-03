package com.nyu.zyxt.Services;

import com.fasterxml.jackson.databind.JsonNode;

import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class Job {
    private String jobId;
    private String time;
    private String action;
    private String status;
    private double totalNetworkCost;
    private Integer totalTowers;
    private Integer totalLinks;
    private Integer totalRelays;
    private List<EquipmentUsed> equipmentsUsed;
    private String titleString;

    public Job() {
        this.equipmentsUsed = new ArrayList<>();
    }

    public String getJobId() {
        return jobId;
    }

    public void setJobId(String jobId) {
        this.jobId = jobId;
    }

    public String getTime() {
        return time;
    }

    public void setTime(String time) {
        this.time = time;
    }

    public String getAction() {
        return action;
    }

    public void setAction(String action) {
        this.action = action;
    }

    public String getStatus() {
        return status;
    }

    public String getTitleString() {
        return titleString;
    }

    public void setTitleString(String titleString) {
        this.titleString = titleString;
    }

    public void setStatus(String status) {
        this.status = status;
    }

    public double getTotalNetworkCost() {
        return totalNetworkCost;
    }

    public void setTotalNetworkCost(double totalNetworkCost) {
        this.totalNetworkCost = totalNetworkCost;
    }

    public Integer getTotalTowers() {
        return totalTowers;
    }

    public void setTotalTowers(Integer totalTowers) {
        this.totalTowers = totalTowers;
    }

    public Integer getTotalLinks() {
        return totalLinks;
    }

    public void setTotalLinks(Integer totalLinks) {
        this.totalLinks = totalLinks;
    }

    public Integer getTotalRelays() {
        return totalRelays;
    }

    public void setTotalRelays(Integer totalRelays) {
        this.totalRelays = totalRelays;
    }

    public List<EquipmentUsed> getEquipmentsUsed() {
        return equipmentsUsed;
    }

    public void setEquipmentsUsed(List<EquipmentUsed> equipmentsUsed) {
        this.equipmentsUsed = equipmentsUsed;
    }

    public static void parseAndSetCostsOfJob(Job job, JsonNode jobJsonResult, String mainZyxtPath) throws IOException {

        String jobEquipments = mainZyxtPath + "/customerNodes/" + job.getJobId() + "/newInputMapEquipments_"+job.getJobId()+".csv";
        Map<Integer, Equipment> equipmentMap = Equipment.readEquipmentsFromFile(jobEquipments);
        job.setTotalNetworkCost(jobJsonResult.get("equipmentCost").asDouble());
        job.setTotalTowers(jobJsonResult.path("nodes").size());
        job.setTotalLinks(jobJsonResult.path("edges").size());
        JsonNode allNodes = jobJsonResult.path("nodes");
        int relayCount = 0;
        for(JsonNode node : allNodes){
            if(node.get("nodeProperty").get("type").asText().equals("intermediate")){
                relayCount++;
            }
        }
        job.setTotalRelays(relayCount);
        JsonNode allEdges = jobJsonResult.path("edges");
        for(JsonNode edge : allEdges){
           boolean alreadyExist = false;
           for(int i = 0; i < job.getEquipmentsUsed().size(); i++){
               if(job.getEquipmentsUsed().get(i).getEquipment().getId() == edge.get("edgeProperty").get("deviceId").asInt()){
                   EquipmentUsed equipmentUsed = job.getEquipmentsUsed().get(i);
                   equipmentUsed.incrementCounter();
                   alreadyExist = true;
                   break;
               }
           }
           if(!alreadyExist){
               Equipment findEquipment = equipmentMap.get(edge.get("edgeProperty").get("deviceId").asInt());
               if(findEquipment != null){
                   EquipmentUsed equipmentUsed = new EquipmentUsed();
                   equipmentUsed.setEquipment(findEquipment);
                   equipmentUsed.incrementCounter();
                   job.getEquipmentsUsed().add(equipmentUsed);
               }
           }
        }
        String html = "";
        for(int i = 0; i < job.getEquipmentsUsed().size(); i++){
            html += (i+1) + ". "+job.getEquipmentsUsed().get(i).getEquipment().getName()+": ("+job.getEquipmentsUsed().get(i).getCount()+" * $"+job.getEquipmentsUsed().get(i).getEquipment().getCost()+" = $"+job.getEquipmentsUsed().get(i).getEquipment().getCost() * job.getEquipmentsUsed().get(i).getCount()+")<br/>";
        }
        job.setTitleString(html);
    }



}
