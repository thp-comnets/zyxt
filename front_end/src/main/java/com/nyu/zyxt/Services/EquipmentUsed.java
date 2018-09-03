package com.nyu.zyxt.Services;

public class EquipmentUsed {
    private Equipment equipment;
    private int count;

    public Equipment getEquipment() {
        return equipment;
    }

    public void setEquipment(Equipment equipment) {
        this.equipment = equipment;
    }

    public int getCount() {
        return count;
    }

    public void setCount(int count) {
        this.count = count;
    }

    public void incrementCounter(){
        this.count++;
    }
}
