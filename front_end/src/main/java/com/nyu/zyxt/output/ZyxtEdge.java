package com.nyu.zyxt.output;

import java.util.List;

public class ZyxtEdge {

    private ZyxtEdgeProprty edgeProperty;
    private List<Integer> nodes;

    public ZyxtEdgeProprty getEdgeProperty() {
        return edgeProperty;
    }

    public void setEdgeProperty(ZyxtEdgeProprty edgeProperty) {
        this.edgeProperty = edgeProperty;
    }

    public List<Integer> getNodes() {
        return nodes;
    }

    public void setNodes(List<Integer> nodes) {
        this.nodes = nodes;
    }
}
