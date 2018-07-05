package com.nyu.zyxt.Services;

public class ZyxtInput {

    private Double lat;
    private Double lng;
    private String type;
    private int capacity;
    private int height;

    public ZyxtInput() {
    }

    public ZyxtInput(Double lat, Double lng) {
        this.lat = lat;
        this.lng = lng;
    }

    public int getHeight() {
        return height;
    }

    public void setHeight(int height) {
        this.height = height;
    }

    public Double getLat() {
        return lat;
    }

    public void setLat(Double lat) {
        this.lat = lat;
    }

    public Double getLng() {
        return lng;
    }

    public void setLng(Double lng) {
        this.lng = lng;
    }

    public String getType() {
        return type;
    }

    public void setType(String type) {
        this.type = type;
    }

    public int getCapacity() {
        return capacity;
    }

    public void setCapacity(int capacity) {
        this.capacity = capacity;
    }
}
