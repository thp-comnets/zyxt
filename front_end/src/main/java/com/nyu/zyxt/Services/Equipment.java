package com.nyu.zyxt.Services;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class Equipment {
    private int id;
    private String name;
    private double throughput;
    private double range;
    private double cost;
    private boolean selected;


    public int getId() {
        return id;
    }

    public Equipment() {
        this.selected = true;
    }

    public boolean isSelected() {
        return selected;
    }

    public void setSelected(boolean selected) {
        this.selected = selected;
    }

    public void setId(int id) {
        this.id = id;
    }

    public String getName() {
        return name;
    }

    public void setName(String name) {
        this.name = name;
    }

    public double getThroughput() {
        return throughput;
    }

    public void setThroughput(double throughput) {
        this.throughput = throughput;
    }

    public double getRange() {
        return range;
    }

    public void setRange(double range) {
        this.range = range;
    }

    public double getCost() {
        return cost;
    }

    public void setCost(double cost) {
        this.cost = cost;
    }

    public static Map<Integer, Equipment> readEquipmentsFromFile(String jobEquipments) throws IOException {
        BufferedReader bufferedReader = new BufferedReader(new FileReader(new File(jobEquipments)));
        List<String> allLines = new ArrayList<>();
        String line;
        while ((line = bufferedReader.readLine()) != null) {
            allLines.add(line);
        }
        bufferedReader.close();
        Map<Integer, Equipment> equipmentMap = new HashMap<>();
        for (String line_ : allLines){
            String [] equipmentParts = line_.split(",");
            Equipment eq = new Equipment();
            eq.setName(equipmentParts[0]);
            eq.setThroughput(Double.parseDouble(equipmentParts[1]));
            eq.setRange(Double.parseDouble(equipmentParts[2]));
            eq.setCost(Double.parseDouble(equipmentParts[3]));
            eq.setId(Integer.parseInt(equipmentParts[4]));
            eq.setSelected(Boolean.parseBoolean(equipmentParts[5]));
            equipmentMap.put(eq.getId(), eq);
        }
        return equipmentMap;
    }
}
