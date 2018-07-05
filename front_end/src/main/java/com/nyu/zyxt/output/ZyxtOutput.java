package com.nyu.zyxt.output;

import java.util.List;

public class ZyxtOutput {

    private int bandwidth;
    private double costs;
    private double equipmentCost;
    private double originalCost;
    private int time;
    private List<ZyxtEdge> edges;
    private List<ZyxtNode> nodes;

    public int getBandwidth() {
        return bandwidth;
    }

    public void setBandwidth(int bandwidth) {
        this.bandwidth = bandwidth;
    }

    public double getCosts() {
        return costs;
    }

    public void setCosts(double costs) {
        this.costs = costs;
    }

    public double getEquipmentCost() {
        return equipmentCost;
    }

    public void setEquipmentCost(double equipmentCost) {
        this.equipmentCost = equipmentCost;
    }

    public double getOriginalCost() {
        return originalCost;
    }

    public void setOriginalCost(double originalCost) {
        this.originalCost = originalCost;
    }

    public List<ZyxtEdge> getEdges() {
        return edges;
    }

    public void setEdges(List<ZyxtEdge> edges) {
        this.edges = edges;
    }

    public List<ZyxtNode> getNodes() {
        return nodes;
    }

    public void setNodes(List<ZyxtNode> nodes) {
        this.nodes = nodes;
    }

    public int getTime() {
        return time;
    }

    public void setTime(int time) {
        this.time = time;
    }


}
