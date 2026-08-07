package com.example.homewallcontroller2026;

public class RecentClimb {

    private final String name;
    private final int level;
    private final String timestamp;

    public RecentClimb(String name, int level, String timestamp) {
        this.name = name;
        this.level = level;
        this.timestamp = timestamp;
    }

    public String getName() {
        return name;
    }

    public int getLevel() {
        return level;
    }

    public String getTimestamp() {
        return timestamp;
    }

    @Override
    public String toString() {
        return "Level " + level + " — " + name;
    }
}