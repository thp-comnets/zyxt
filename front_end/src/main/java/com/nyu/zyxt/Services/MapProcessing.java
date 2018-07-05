package com.nyu.zyxt.Services;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;

public class MapProcessing {
    private List<ZyxtInput> zyxtInputList;
    private ZyxtInput tLeft;
    private ZyxtInput tRight;
    private ZyxtInput bLeft;
    private ZyxtInput bRight;
    private double mapWidth;
    private double mapHeight;
    private double expectedRows;
    private double expectedCols;

    Double getExpectedRows() {
        return expectedRows;
    }

    Double getExpectedCols() {
        return expectedCols;
    }


    private Logger logger = LoggerFactory.getLogger(this.getClass());

    MapProcessing(List<ZyxtInput> zyxtInputList) {
        this.zyxtInputList = zyxtInputList;
        this.tLeft = new ZyxtInput();
        this.tRight = new ZyxtInput();
        this.bLeft = new ZyxtInput();
        this.bRight = new ZyxtInput();
    }

    private double deg2rad(double deg){
        return deg * (Math.PI / 180);
    }

    private double calcuateDistance(ZyxtInput p1, ZyxtInput p2){
        double R = 6371; // Radius of the earth in km
        double dLat = deg2rad(p2.getLat() - p1.getLat());  // deg2rad below
        double dLon = deg2rad(p2.getLng() - p1.getLng());
        double a = Math.sin(dLat / 2) * Math.sin(dLat/2) + Math.cos(deg2rad(p1.getLat())) * Math.cos(deg2rad(p2.getLat())) * Math.sin( dLon / 2 ) * Math.sin( dLon / 2 );
        double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1-a));
        return (R * c); // Distance in km
    }

    private void calculateBounds(){
        ZyxtInput minx = new ZyxtInput(Double.MAX_VALUE,0.0);
        ZyxtInput miny = new ZyxtInput(0.0, Double.MAX_VALUE);
        ZyxtInput maxx = new ZyxtInput(-Double.MAX_VALUE-1, 0.0);
        ZyxtInput maxy = new ZyxtInput(0.0, -Double.MAX_VALUE-1);
        for(ZyxtInput coord : this.zyxtInputList){
            if (coord.getLat() < minx.getLat()){
                minx = coord;
            }
            if(coord.getLng() < miny.getLng()){
                miny = coord;
            }
            if(coord.getLat() > maxx.getLat()){
                maxx = coord;
            }
            if(coord.getLng() > maxy.getLng()){
                maxy = coord;
            }
        }
        this.tLeft.setLat(maxx.getLat());
        this.tLeft.setLng(miny.getLng());
        this.tRight.setLat(maxx.getLat());
        this.tRight.setLng(maxy.getLng());
        this.bLeft.setLat(minx.getLat());
        this.bLeft.setLng(miny.getLng());
        this.bRight.setLat(minx.getLat());
        this.bRight.setLng(maxy.getLng());
    }

    public double getAreaOfMap(){
        return this.mapWidth * this.mapHeight;
    }

    void processMap(){
        this.calculateBounds();
        this.mapWidth = calcuateDistance(this.tLeft, this.tRight);
        this.mapHeight = calcuateDistance(this.tLeft, this.bLeft);
        this.expectedRows = (this.mapHeight * 1000) / 30.0;
        this.expectedCols = (this.mapWidth * 1000) / 25.0;
        logger.info("Area: " + this.mapWidth + " x " + this.mapHeight + ": " + (this.mapWidth * this.mapHeight) + " Km^2");
        logger.info("Rows: " + this.expectedRows + ", Cols: " + this.expectedCols + ", rows*cols: " + (this.expectedRows * this.expectedCols));

    }

}
