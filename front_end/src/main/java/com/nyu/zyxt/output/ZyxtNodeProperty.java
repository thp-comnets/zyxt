package com.nyu.zyxt.output;

public class ZyxtNodeProperty {

    private int capacity;
    private double lat;
    private double lng;
    private int mountingHeight;
    private String type;

    public int getCapacity() {
        return capacity;
    }

    public void setCapacity(int capacity) {
        this.capacity = capacity;
    }

    public double getLat() {
        return lat;
    }

    public void setLat(double lat) {
        this.lat = lat;
    }

    public double getLng() {
        return lng;
    }

    public void setLng(double lng) {
        this.lng = lng;
    }

    public int getMountingHeight() {
        return mountingHeight;
    }

    public void setMountingHeight(int mountingHeight) {
        this.mountingHeight = mountingHeight;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }
}
