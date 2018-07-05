package com.nyu.zyxt.output;

public class ZyxtEdgeProprty {

    private int bandwidth;
    private int deviceCost;
    private int flowPassed;
    private int unitCost;
    private String length;
    private String frequency;

    public int getBandwidth() {
        return bandwidth;
    }

    public void setBandwidth(int bandwidth) {
        this.bandwidth = bandwidth;
    }

    public int getUnitCost() {
        return unitCost;
    }

    public void setUnitCost(int unitCost) {
        this.unitCost = unitCost;
    }

    public int getDeviceCost() {
        return deviceCost;
    }

    public void setDeviceCost(int deviceCost) {
        this.deviceCost = deviceCost;
    }

    public int getFlowPassed() {
        return flowPassed;
    }

    public void setFlowPassed(int flowPassed) {
        this.flowPassed = flowPassed;
    }

    public String getLength() {
        return length;
    }

    public void setLength(String length) {
        this.length = length;
    }

    public String getFrequency() {
        return frequency;
    }

    public void setFrequency(String frequency) {
        this.frequency = frequency;
    }
}
