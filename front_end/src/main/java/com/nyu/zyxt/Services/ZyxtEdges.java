package com.nyu.zyxt.Services;

public class ZyxtEdges {
    private double lat1;
    private double lng1;
    private double lat2;
    private double lng2;
    private Equipment device;

    public Equipment getDevice() {
        return device;
    }

    public void setDevice(Equipment device) {
        this.device = device;
    }

    public double getLat1() {
        return lat1;
    }

    public void setLat1(double lat1) {
        this.lat1 = lat1;
    }

    public double getLng1() {
        return lng1;
    }

    public void setLng1(double lng1) {
        this.lng1 = lng1;
    }

    public double getLat2() {
        return lat2;
    }

    public void setLat2(double lat2) {
        this.lat2 = lat2;
    }

    public double getLng2() {
        return lng2;
    }

    public void setLng2(double lng2) {
        this.lng2 = lng2;
    }
}
