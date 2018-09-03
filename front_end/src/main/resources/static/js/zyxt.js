$('#selected_equipments').slimScroll({
    height: '400px'
});



let map;
let markerType = -1;
let isAddEdge = 0;
let edgePath = [];
let marker = null;
let markerCounter = 0;
let currentPosLatLng;
let updatingMarker = null;
let removingMarker = null;
let g = new jsnx.MultiDiGraph();
let allEquipments = [];
let selectedEquipments = [];


$("#sink_cap").editable({
    mode: "inline",
    showbuttons: false,
    validate: function(value) {
        if($.trim(value) === '') {
            return 'Bandwidth is required';
        }
    }
}).on('shown', function(e, editable) {
    editable.input.$input.attr("size", "1");
}).on('save', function(e, params) {
    $("#sinkCapacity").val(params.newValue);
});

$("#mounting_default_html").editable({
    mode: "inline",
    showbuttons: false,
    validate: function(value) {
        if($.trim(value) === '') {
            return 'Mounting Height is required';
        }
    }
}).on('shown', function(e, editable) {
    editable.input.$input.attr("size", "1");
}).on('save', function(e, params) {
    $("#mountingDefaultInput").val(params.newValue);
});

$(".eq_group_item").each(function(){
   let equipmentObj = {
       id: $(this).children().eq(1).val(),
       throughput: $(this).children().eq(2).val(),
       name: $(this).children().eq(3).val(),
       range: $(this).children().eq(4).val(),
       cost: $(this).children().eq(5).val()
   };
   allEquipments.push(equipmentObj);
   selectedEquipments.push(equipmentObj);
    initilizeDeviceSelectList();
});

function makeBtnSelect(btn){
    if(btn !== null && btn.hasClass("btn-success")){
        $(btn).removeClass("btn-success");
        $(btn).addClass("btn-primary");
        markerType = -1;
        isAddEdge = 0;
    }else{
        $('.m_btn').each(function(i, obj) {
            if(btn !=null && btn.html() === $(this).html()){
                $(this).removeClass("btn-primary");
                $(this).addClass("btn-success");
            }else{
                $(this).removeClass("btn-success");
                $(this).addClass("btn-primary");
            }
        });
    }

}

$(".sourceBtn").click(function(){
    markerType = 0;
    isAddEdge = 0;
    makeBtnSelect($(this));
});
$(".sinkBtn").click(function(){
    markerType = 1;
    isAddEdge = 0;
    makeBtnSelect($(this));
});
$(".add_link").click(function(){
    markerType = -1;
    isAddEdge = 1;
    makeBtnSelect($(this));
});

function getNextNodeIndex(){
    let allNodes = g.nodes(true);
    let nextIndex = 0;
    for(let i = 0; i < allNodes.length; i++){
        if (allNodes[i][0] > nextIndex){
            nextIndex = allNodes[i][0];
        }
    }
    return nextIndex+1;
}


let inputJson = {};
function initMap() {
    map = new google.maps.Map(document.getElementById('map'), {
        center: {lat: 38.4615244, lng: -122.93603567},
        scaleControl: true,
        mapTypeId: 'terrain',
        zoom: 8
    });
    map.addListener('click', function(event) {
        let nextNodeIndex = getNextNodeIndex();
        if(markerType === 0){
            if(isSourceExist()){
                alertify.set('notifier','position', 'top-right');
                alertify.warning("Only One Source Supported");
                markerType = -1;
                return false;
            }

            console.log("Creating Source");
            currentPosLatLng = event.latLng;
            marker = new google.maps.Marker({
                position: currentPosLatLng,
                title:"Source",
                icon: "/img/source.png",
                id: markerCounter,
                draggable:true,
                connected: false,
                graphNode: nextNodeIndex
            });
            marker.setMap(map);
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent($("#sinkCapacity").val(), $("#mountingDefaultInput").val(), "source")
            });
            var resObj = {marker: marker, infoWindow: infowindow, mounting:$("#mountingDefaultInput").val(), capacity:$("#sinkCapacity").val(),type:"source"};
            g.addNode(nextNodeIndex, {prop:{capacity:$("#sinkCapacity").val(),lat:marker.getPosition().lat(), lng: marker.getPosition().lng(), mountingHeight:$("#mountingDefaultInput").val(), type:"source"}, marker:marker});
            inputJson[markerCounter] = resObj;
            markerCounter++;
            initializeMarker(marker);

        }else if(markerType === 1){
            let nextNodeIndex = getNextNodeIndex();
            currentPosLatLng = event.latLng;
            console.log("Creating Sink");
            marker = new google.maps.Marker({
                position: currentPosLatLng,
                title:"Sink",
                icon: "/img/sink.png",
                id: markerCounter,
                draggable:true,
                connected: false,
                graphNode: nextNodeIndex
            });
            marker.setMap(map);
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent($("#sinkCapacity").val(), $("#mountingDefaultInput").val(), "sink")
            });
            var resObj = {marker: marker,infoWindow: infowindow, mounting:$("#mountingDefaultInput").val(), capacity:$("#sinkCapacity").val(),type:"sink"};
            g.addNode(nextNodeIndex, {prop:{capacity:$("#sinkCapacity").val(),lat:marker.getPosition().lat(), lng: marker.getPosition().lng(), mountingHeight:$("#mountingDefaultInput").val(), type:"sink"}, marker:marker});
            inputJson[markerCounter] = resObj;
            markerCounter++;
            initializeMarker(marker);
        }
        console.log(inputJson);
        console.log("Nodes\n");
        console.log(g.nodes(true));
    });
    $('#cap_modal').on('hidden.bs.modal', function (e) {
        if(removingMarker !== null){
            console.log("Removing Marker");
            var markerObj = inputJson[removingMarker];
            g.removeNode(markerObj.marker.graphNode);
            markerObj.marker.setMap(null);
            delete inputJson[removingMarker];
        }else if(updatingMarker !== null){
            console.log("Updating Marker");
            let markerObj = inputJson[updatingMarker];
            markerObj.capacity = $("#inputCapacity").val();
            markerObj.mounting = $("#mountingHeight").val();
            let nodeProp = getNodePropertiesFromGraph(markerObj.marker.graphNode);
            nodeProp.capacity = $("#inputCapacity").val();
            nodeProp.mountingHeight = $("#mountingHeight").val();
            var type = markerObj.type;
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent($("#inputCapacity").val(), $("#mountingHeight").val(), type)
            });
            markerObj.infoWindow = infowindow;
        }
        console.log(inputJson);
        console.log("Nodes\n");
        console.log(g.nodes(true));
        $("#inputCapacity").val("");
        $("#mountingHeight").val("");
        updatingMarker = null;
        removingMarker = null;
    });

    $(".remove_btn").click(function(){
        removingMarker = updatingMarker;
        $("#cap_modal").modal("hide");
    });

    $(".draw_on_maps").click(function(){
        for (var key in inputJson){
            var obj = inputJson[key];
            obj.marker.setMap(null);
        }
        inputJson = {};
        markerCounter = 0;
        let bounds = new google.maps.LatLngBounds();
        let totalSourceCapacity = 0;
        if($('.sinks').val() !== ""){
            let sinksLocations = $('.sinks').val().split(/\n/);
            for(let i = 0; i < sinksLocations.length; i++){
                let nextNodeIndex = getNextNodeIndex();
                var locData = sinksLocations[i].split(",");
                console.log("Creating Sink");
                var sinkPos = new google.maps.LatLng(parseFloat(locData[0]), parseFloat(locData[1]));
                bounds.extend(sinkPos);
                marker = new google.maps.Marker({
                    position: sinkPos,
                    title:"Sink",
                    icon: "/img/sink.png",
                    id: markerCounter,
                    draggable:true,
                    connected: false,
                    graphNode: nextNodeIndex
                });
                marker.setMap(map);
                var infowindow = new google.maps.InfoWindow({
                    content: setInfoWindowContent(parseInt(locData[2]), parseInt(locData[3]), "sink")
                });
                totalSourceCapacity += parseInt(locData[2]);
                var resObj = {marker: marker,infoWindow: infowindow, mounting:parseInt(locData[3]), capacity:parseInt(locData[2]),type:"sink"};
                g.addNode(nextNodeIndex, {prop:{capacity:parseInt(locData[2]),lat:marker.getPosition().lat(), lng: marker.getPosition().lng(), mountingHeight:parseInt(locData[3]), type:"sink"}, marker:marker});
                inputJson[markerCounter] = resObj;
                markerCounter++;
                initializeMarker(marker);
            }
        }
        if($(".source").val() !== ""){
            let nextNodeIndex = getNextNodeIndex();
            var sourceLocation = $(".source").val().split(",");
            console.log(sourceLocation);
            console.log("Creating Source");
            var sourcePos = new google.maps.LatLng(parseFloat(sourceLocation[0]), parseFloat(sourceLocation[1]));
            bounds.extend(sourcePos);
            marker = new google.maps.Marker({
                position: sourcePos,
                title:"Source",
                icon: "/img/source.png",
                id: markerCounter,
                draggable:true,
                connected: false,
                graphNode: nextNodeIndex
            });
            marker.setMap(map);
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent(totalSourceCapacity, parseInt(sourceLocation[2]), "source")
            });
            var resObj = {marker: marker, infoWindow: infowindow, mounting:parseInt(sourceLocation[2]), capacity:totalSourceCapacity,type:"source"};
            g.addNode(nextNodeIndex, {prop:{capacity:totalSourceCapacity,lat:marker.getPosition().lat(), lng: marker.getPosition().lng(), mountingHeight:parseInt(sourceLocation[2]), type:"source"}, marker:marker});
            inputJson[markerCounter] = resObj;
            markerCounter++;
            initializeMarker(marker);
        }
        $(".sinks").val("");
        $(".source").val("");
        $("#location_data").modal("hide");
        map.fitBounds(bounds);
        console.log("Nodes\n");
        console.log(g.nodes(true));
        console.log(inputJson);
    });


// Create the search box and link it to the UI element.
    let input = document.getElementById('pac-input');
    let searchBox = new google.maps.places.SearchBox(input);
    map.controls[google.maps.ControlPosition.TOP_CENTER].push(input);

// // Bias the SearchBox results towards current map's viewport.
    map.addListener('bounds_changed', function() {
        searchBox.setBounds(map.getBounds());
    });

    searchBox.addListener('places_changed', function() {
        let places = searchBox.getPlaces();

        if (places.length === 0) {
            return;
        }


        // For each place, get the icon, name and location.
        let bounds = new google.maps.LatLngBounds();
        places.forEach(function(place) {
            if (!place.geometry) {
                console.log("Returned place contains no geometry");
                return;
            }

            if (place.geometry.viewport) {
                // Only geocodes have viewport.
                bounds.union(place.geometry.viewport);
            } else {
                bounds.extend(place.geometry.location);
            }
        });
        map.fitBounds(bounds);
    });

    google.maps.event.addListenerOnce(map, 'tilesloaded', function(){
        $("#pac-input").removeClass("hide");
    });

}

function initializeMarker(marker){
    $(".save_btn").html("Update");
    google.maps.event.addListener(marker, 'click', function(event) {
        let markerObj = inputJson[marker.id];
        if(isAddEdge !== 1){
            $("#inputCapacity").val(markerObj.capacity);
            $("#mountingHeight").val(markerObj.mounting);
            $(".modal-title").html(markerObj.type);
            updatingMarker = marker.id;
            if(marker.connected){
                $(".remove_btn").addClass("hide");
                $(".free_btn").removeClass("hide").attr("data-graph-node", marker.graphNode);
            }else{
                $(".remove_btn").removeClass("hide");
                $(".free_btn").addClass("hide");
            }
            $("#cap_modal").modal("show");
        }else{
            edgePath.push({lat:markerObj.marker.getPosition().lat(),lng:markerObj.marker.getPosition().lng(), mountingHeight:markerObj.mounting, connected: markerObj.marker.connected, graphNode: markerObj.marker.graphNode, markerId:markerObj.marker.id});
            marker.setAnimation(google.maps.Animation.BOUNCE);
            if(edgePath.length === 2){
                inputJson[edgePath[0]["markerId"]].marker.setAnimation(null);
                inputJson[edgePath[1]["markerId"]].marker.setAnimation(null);
                console.log(edgePath);
                getElevationOfPath();
            }
        }

    });
    google.maps.event.addListener(marker, 'mouseover', function(event) {
        var markerObj = inputJson[marker.id];
        markerObj.infoWindow.open(map, marker);
    });
    google.maps.event.addListener(marker, 'mouseout', function(event) {
        var markerObj = inputJson[marker.id];
        markerObj.infoWindow.close();
    });

    google.maps.event.addListener(marker, 'mouseup', function(event) {
        let markerObj = inputJson[marker.id];
        let allNodes = g.nodes(true);
        console.log(allNodes);
        for(let i = 0; i < allNodes.length; i++){
            if(allNodes[i][0] === markerObj.marker.graphNode){
                console.log(markerObj.marker.graphNode);
                allNodes[i][1]["prop"]["lat"] = marker.getPosition().lat();
                allNodes[i][1]["prop"]["lng"] = marker.getPosition().lng();
            }
        }
        console.log(g.nodes(true));
    });
}

function setInfoWindowContent(capacity, mountingHeight, type) {
    var content = '';
    var className = (type === "source") ? "text-danger" : "text-success";
    content += '<ul>Type: <b class="'+className+'">'+type+'</b></ul>';
    if(type !== "source"){
        content += '<ul> Demand: ' + capacity + ' Mbit/s </ul>';
    }


    content += '<ul> Mounting Height: ' + mountingHeight + ' m </ul>';


    return content
}


function setInfoWindowContentForEdge(edgeProp) {
    var content = '';
    var className = "text-info";
    content += '<ul>Bandwidth: <b class="'+className+'">'+edgeProp["bandwidth"]+' Mbit/s</b></ul>';

    content += '<ul>Device Cost: <b class="'+className+'">$'+edgeProp["deviceCost"]+' </b></ul>';

    content += '<ul>Flow Passed: <b class="'+className+'">'+edgeProp["flowPassed"]+' Mbit/s</b></ul>';

    content += '<ul>Length: <b class="'+className+'">'+edgeProp["length"]+'</b></ul>';

    // content += '<ul> Mounting Height: ' + mountingHeight + ' m </ul>';


    return content
}


function isSourceExist(){
    for(var i in inputJson){
        if(inputJson[i].type === "source"){
            return true;
        }
    }
    return false;
}

function calculateDistance(lat1, lon1, lat2, lon2, unit){
    const radlat1 = Math.PI * lat1/180;
    const radlat2 = Math.PI * lat2/180;
    const theta = lon1-lon2;
    const radtheta = Math.PI * theta/180;
    let dist = Math.sin(radlat1) * Math.sin(radlat2) + Math.cos(radlat1) * Math.cos(radlat2) * Math.cos(radtheta);
    dist = Math.acos(dist);
    dist = dist * 180/Math.PI;
    dist = dist * 60 * 1.1515;
    if (unit==="K") { dist = dist * 1.609344 }
    if (unit==="N") { dist = dist * 0.8684 }
    return dist
}

function calculateGradientOnLine(observer, target, withoutTargetMountingHeight){
    const distanceFromObserverToTarget = calculateDistance(observer.lat, observer.lng, target.lat, target.lng, "K");
    const targetTotalElevation = (!withoutTargetMountingHeight) ? target.elevation + target.mountingHeight : target.elevation;
    const observerTotalElevation = observer.elevation + observer.mountingHeight;
    return (targetTotalElevation - observerTotalElevation) / distanceFromObserverToTarget;
}


function getElevationOfPath(){
    // if(edgePath[0]["connected"]){
    //     alertify.error("First, Disconnect The Observer Marker!");
    //     resetAddEdgeSettings();
    //     return;
    // }
    let elevator = new google.maps.ElevationService;
    elevator.getElevationAlongPath({
        'path': edgePath,
        'samples': 256
    }, elevationResponse);
}

function elevationResponse(elevations, status){
    // console.log(status);
    // console.log(elevations);

    if(status !== "OK"){
        $("#output").html("<b class='text-danger'>Unable to add link due to: " + status + ". Please try again.</b>");
        resetAddEdgeSettings();
        return;
    }
    let observer = {lat: edgePath[0]["lat"], lng: edgePath[0]["lng"], mountingHeight: parseInt(edgePath[0]["mountingHeight"]), elevation: elevations[0].elevation};
    let maxGradientBetweenObserverAndTarget = Number.MIN_VALUE;
    let observerCanSeeLastTarget = false;
    for (let i = 1; i < elevations.length; i++) {
        // console.log(elevations[i].elevation+" :: "+elevations[i].location.lat() + ", " + elevations[i].location.lng());
        let target = {};
        if (i === elevations.length-1){
            target = {lat: elevations[i].location.lat(), lng: elevations[i].location.lng(), mountingHeight: parseInt(edgePath[1]["mountingHeight"]), elevation: elevations[i].elevation};
        }else{
            target = {lat: elevations[i].location.lat(), lng: elevations[i].location.lng(), mountingHeight: parseInt($("#mountingDefaultInput").val()), elevation: elevations[i].elevation};
        }
        const gradient = calculateGradientOnLine(observer, target, false);
        const gradientWithoutTowerHeight = calculateGradientOnLine(observer, target, true);
        if(gradient >= maxGradientBetweenObserverAndTarget){
            maxGradientBetweenObserverAndTarget = gradientWithoutTowerHeight;
            observerCanSeeLastTarget = true;
        }else{
            observerCanSeeLastTarget = false;
        }
    }
    if(observerCanSeeLastTarget){
        let line = new google.maps.Polyline({
            path: edgePath,
            geodesic: true,
            strokeColor: '#0c53af',
            strokeOpacity: 0.8,
            strokeWeight: 5,
            from: edgePath[0]["graphNode"],
            to: edgePath[1]["graphNode"]
        });
        inputJson[ edgePath[0]["markerId"] ].marker.connected = true;
        inputJson[ edgePath[1]["markerId"] ].marker.connected = true;
        inputJson[ edgePath[0]["markerId"] ].marker.setDraggable(false);
        inputJson[ edgePath[1]["markerId"] ].marker.setDraggable(false);
        g.addEdge(edgePath[0]["graphNode"], edgePath[1]["graphNode"], {line:line, 'device':{}});
        line.setMap(map);
        google.maps.event.addListener(line, 'click', function(event) {
            let edge = getEdgeFromGrpah(line.from, line.to);
            console.log(edge);
            $("#associated_device_select").prev().prev().val(line.from);
            $("#associated_device_select").prev().val(line.to);
            $("#associated_device_select").val(getIndexOfSelectEquipment(edge[2]['device']['id']));
            initDeviceMetaData(getIndexOfSelectEquipment(edge[2]['device']['id']));
            $("#associate_device_modal").modal("show");
        });
    }else{
        alertify.set('notifier','position', 'top-center');
        alertify.error("Can't connect these sites due to line of sight. Try increasing the mounting height");
    }
    console.log("Edges\n");
    console.log(g.edges(true));
    resetAddEdgeSettings();
}

function getIndexOfSelectEquipment(equipmentID){
    if(equipmentID === undefined){
        return -1;
    }
    for(let i = 0; i < selectedEquipments.length; i++){
        if(selectedEquipments[i].id === equipmentID){
            return i;
        }
    }

}

//associate device with the link
$(".associate_device_btn").click(function (){
    let selectedDeviceId = parseInt($("#associated_device_select").val());
    let allEdges = g.edges(true);
    let from = parseInt($("#associated_device_select").prev().prev().val());
    let to = parseInt($("#associated_device_select").prev().val());
    for (let ed in allEdges){
        if(allEdges[ed][0] === from && allEdges[ed][1] === to){
            allEdges[ed][2]['device'] = selectedEquipments[selectedDeviceId];
            break;
        }
    }
    console.log(g.edges(true));
    $("#associate_device_modal").modal("hide");
});

//remove pinned link
$(".re_associate_device_btn").click(function(){
    let allEdges = g.edges(true);
    let from = parseInt($("#associated_device_select").prev().prev().val());
    let to = parseInt($("#associated_device_select").prev().val());
    for (let ed in allEdges){
        if(allEdges[ed][0] === from && allEdges[ed][1] === to){
            allEdges[ed][2]['line'].setMap(null);
            g.removeEdge(from, to);
            break;
        }
    }
    console.log(g.edges(true));
    $("#associate_device_modal").modal("hide");
});

function resetAddEdgeSettings(){
    edgePath = [];
    isAddEdge = 0;
    makeBtnSelect(null);
}


function getPropertiesOfNode(nodeToCheck, obj){
    for(var i = 0; i < obj.nodes.length; i++){
        if(obj.nodes[i].node === nodeToCheck){
            return obj.nodes[i]["nodeProperty"];
        }
    }
}

Object.size = function(obj) {
    var size = 0, key;
    for (key in obj) {
        if (obj.hasOwnProperty(key)) size++;
    }
    return size;
};

function getNodePropertiesFromGraph(node){
    var allNodes = g.nodes(true);
    for(var i = 0; i < allNodes.length; i++){
        if(allNodes[i][0] === node){
            return allNodes[i][1]["prop"];
        }
    }
}

function getNodeMarkerFromGraph(node){
    var allNodes = g.nodes(true);
    for(var i = 0; i < allNodes.length; i++){
        if(allNodes[i][0] === node){
            return allNodes[i][1]["marker"];
        }
    }
}

function getEdgeFromGrpah(from, to){
    let allEdges = g.edges(true);
    for (let e in allEdges){
        if(allEdges[e][0] === from && allEdges[e][1] === to){
            return allEdges[e];
        }
    }
}

function resetMarker(node){
    getNodeMarkerFromGraph(parseInt(node)).connected = false;
    getNodeMarkerFromGraph(parseInt(node)).setDraggable(true);
}

function removeEmptyIntermediateNodes(){
    let allNodes = g.nodes(true);
    for(let i = 0; i < allNodes.length; i++){
        let outEdges = g.outDegree(allNodes[i][0]);
        let inEdges = g.inDegree(allNodes[i][0]);
        let prop = allNodes[i][1]["prop"];
        if(outEdges === 0 && inEdges === 0 && prop["type"] === "intermediate"){
            g.removeNode(allNodes[i][0]);
            allNodes[i][1]["marker"].setMap(null);
        }
    }
}

$(".free_btn").click(function(){
    let q = new Queue();
    let node = $(this).attr("data-graph-node");
    let nodeStack = [];
    q.enqueue(node);
    nodeStack.push(parseInt(node));
    // console.log(g.edges(true));
    //removing outgoing
    while(nodeStack.length !== 0){
        const nodeToCheck = nodeStack.pop();
        let outEdges = g.outEdges(nodeToCheck, true);
        for(let i = 0; i < outEdges.length; i++){
            g.removeEdge(outEdges[i][0], outEdges[i][1]);
            outEdges[i][2]["line"].setMap(null);
            nodeStack.push(outEdges[i][1]);
        }
        resetMarker(nodeToCheck);
    }
    //removing incoming;
    while(q.size() !== 0){
        let nodeToCheck = q.dequeue();
        let inEdges = g.inEdges(parseInt(nodeToCheck), true);
        for(let i = 0; i < inEdges.length; i++){
            g.removeEdge(inEdges[i][0], inEdges[i][1]);
            inEdges[i][2]["line"].setMap(null);
            let nodeProp = getNodePropertiesFromGraph(inEdges[i][0]);
            let outEdges = g.outEdges(inEdges[i][0]);
            if(nodeProp["type"] === "intermediate" && outEdges.length === 0){
                q.enqueue(inEdges[i][0]);
            }
        }
    }
    // console.log(g.edges(true));
    console.log("Edges\n");
    console.log(g.edges(true));
    removeEmptyIntermediateNodes();
    resetMarker(node);
    $("#cap_modal").modal("hide");
});


function showOutput(outputJson, defaultMountingHeight){
    var obj = jQuery.parseJSON( outputJson );
    var html = "<li><b>Network Summary:</b></li>";
    html += "<li>Total Network Cost: <b class='badge'>$"+obj.equipmentCost+"</b></li>";
    html += "<li>Total Towers: <b class='badge'>"+obj.nodes.length+"</b></li>";
    html += "<li>Total Links: <b class='badge'>"+obj.edges.length+"</b></li>";
    var relayTowers = 0;
    for(var i = 0; i < obj.nodes.length; i++){
        if(obj.nodes[i]["nodeProperty"]["type"] === "intermediate"){
            relayTowers++;
        }
    }
    html += "<li>Relays: <b class='badge'>"+relayTowers+"</b></li>";
    $("#networkSummary").html(html);
    var processNodes = {};

    var bounds = new google.maps.LatLngBounds();
    for(var i = 0; i < obj.edges.length; i++){
        // console.log(obj.edges[i].nodes[0] + " => " + obj.edges[i].nodes[1]);
        if(processNodes[ obj.edges[i].nodes[0] ] === undefined){
            var nodeProp = getPropertiesOfNode(obj.edges[i].nodes[0], obj);
            bounds.extend(new google.maps.LatLng(nodeProp.lat, nodeProp.lng));
            var markerIcon = (nodeProp.type === "sink") ? "/img/sink.png" : (nodeProp.type === "source") ? "/img/source.png" : "/img/intermediate.png";
            marker = new google.maps.Marker({
                position: {lat: nodeProp.lat, lng: nodeProp.lng},
                title: nodeProp.type,
                icon: markerIcon,
                id: markerCounter,
                connected: true,
                graphNode: obj.edges[i].nodes[0]
            });
            marker.setMap(map);
            g.addNode(obj.edges[i].nodes[0], {prop:nodeProp,marker:marker});
            processNodes[ obj.edges[i].nodes[0] ] = nodeProp;
            //adding in input array
            if(nodeProp.type !== "intermediate"){
                let mountingHeight = (nodeProp.mountingHeight === 0) ? defaultMountingHeight : nodeProp.mountingHeight;
                let nodeCapacity = (nodeProp.capacity < 1) ? nodeProp.capacity * -1 : nodeProp.capacity;
                let infowindow = new google.maps.InfoWindow({
                    content: setInfoWindowContent(nodeCapacity, mountingHeight, nodeProp.type)
                });
                var resObj = {marker: marker,infoWindow: infowindow, mounting:mountingHeight, capacity:nodeCapacity,type:nodeProp.type};
                inputJson[markerCounter] = resObj;
                markerCounter++;
                initializeMarker(marker);
            }

        }
        if(!processNodes[ obj.edges[i].nodes[1] ]){
            var nodeProp = getPropertiesOfNode(obj.edges[i].nodes[1], obj);
            bounds.extend(new google.maps.LatLng(nodeProp.lat, nodeProp.lng));
            var markerIcon = (nodeProp.type === "sink") ? "/img/sink.png" : (nodeProp.type === "source") ? "/img/source.png" : "/img/intermediate.png";
            // console.log(obj.edges[i].nodes[1]);
            marker = new google.maps.Marker({
                position: {lat: nodeProp.lat, lng: nodeProp.lng},
                title: nodeProp.type,
                icon: markerIcon,
                id: markerCounter,
                connected: true,
                graphNode: obj.edges[i].nodes[1]
            });
            marker.setMap(map);
            g.addNode(obj.edges[i].nodes[1], {prop:nodeProp,marker:marker});
            processNodes[ obj.edges[i].nodes[1] ] = nodeProp;
            //adding in input array
            if(nodeProp.type !== "intermediate"){
                var mountingHeight = (nodeProp.mountingHeight === 0) ? defaultMountingHeight : nodeProp.mountingHeight;
                var nodeCapacity = (nodeProp.capacity < 1) ? nodeProp.capacity * -1 : nodeProp.capacity;
                var infowindow = new google.maps.InfoWindow({
                    content: setInfoWindowContent(nodeCapacity, mountingHeight, nodeProp.type)
                });
                var resObj = {marker: marker,infoWindow: infowindow, mounting:mountingHeight, capacity:nodeCapacity,type:nodeProp.type};
                inputJson[markerCounter] = resObj;
                markerCounter++;
                initializeMarker(marker);
            }
        }
        let tail = {lat:  processNodes[ obj.edges[i].nodes[0] ].lat, lng:processNodes[ obj.edges[i].nodes[0] ].lng };
        let head = { lat:  processNodes[ obj.edges[i].nodes[1] ].lat, lng:processNodes[ obj.edges[i].nodes[1] ].lng };
        let line = new google.maps.Polyline({
            path: [tail,head],
            geodesic: true,
            strokeColor: '#0c53af',
            strokeOpacity: 0.8,
            strokeWeight: 5,
            clickable: true,
            from: obj.edges[i].nodes[0],
            to: obj.edges[i].nodes[1],
            edgeProp: obj.edges[i].edgeProperty
        });
        // line.infoWindow = new google.maps.InfoWindow({
        //     content: setInfoWindowContentForEdge(obj.edges[i].edgeProperty)
        // });
        // google.maps.event.addListener(line, 'mouseover', function(event) {
        //     this.infoWindow.open(map);
        //     this.infoWindow.setPosition(event.latLng)
        // });
        // google.maps.event.addListener(line, 'mouseout', function(event) {
        //     this.infoWindow.close();
        // });
        let deviceIndex = getIndexOfSelectEquipment(obj.edges[i].edgeProperty["deviceId"].toString());
        let device = selectedEquipments[deviceIndex];

        g.addEdge(obj.edges[i].nodes[0], obj.edges[i].nodes[1], {line:line, device_cost:obj.edges[i].edgeProperty["deviceCost"], device_id:obj.edges[i].edgeProperty["deviceId"],
            device: device});
        google.maps.event.addListener(line, 'click', function(event) {
            let edge = getEdgeFromGrpah(line.from, line.to);
            console.log(edge);
            $("#associated_device_select").prev().prev().val(line.from);
            $("#associated_device_select").prev().val(line.to);
            $("#associated_device_select").val(getIndexOfSelectEquipment(edge[2]['device']['id']));
            initDeviceMetaData(getIndexOfSelectEquipment(edge[2]['device']['id']), line.edgeProp);
            $("#associate_device_modal").modal("show");
        });

        line.setMap(map);

    }
    map.fitBounds(bounds);
    console.log(inputJson);
    console.log("Nodes\n");
    console.log(g.nodes(true));
    console.log("Edges\n");
    console.log(g.edges(true));
}

function showInput(inputJson){
    let inputObj = jQuery.parseJSON( inputJson );
    console.log(inputObj);
    if(inputObj === null){
        return;
    }
    let bounds = new google.maps.LatLngBounds();
    for(let i = 0; i < inputObj.length; i++){
        if(inputObj[i].type !== "line") {
            let markerIcon = (inputObj[i].type === "sink") ? "/img/sink.png" : "/img/source.png";
            bounds.extend(new google.maps.LatLng(inputObj[i].lat, inputObj[i].lng));
            let marker = new google.maps.Marker({
                position: {lat: inputObj[i].lat, lng: inputObj[i].lng},
                title: inputObj[i].type,
                icon: markerIcon,
            });
            marker.setMap(map);
            let mountingHeight = (inputObj[i].mountingHeight === 0) ? defaultMountingHeight : inputObj[i].mountingHeight;
            let nodeCapacity = (inputObj[i].capacity < 1) ? inputObj[i].capacity * -1 : inputObj[i].capacity;
            let infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent(nodeCapacity, mountingHeight, inputObj[i].type)
            });
            google.maps.event.addListener(marker, 'mouseover', function(event) {
                infowindow.open(map, marker);
            });
            google.maps.event.addListener(marker, 'mouseout', function(event) {
                infowindow.close();
            });
        }else {
            let tail = {lat:  inputObj[i].lat1, lng:inputObj[i].lng1 };
            let head = { lat:  inputObj[i].lat2, lng:inputObj[i].lng2 };
            let line = new google.maps.Polyline({
                path: [tail,head],
                geodesic: true,
                strokeColor: '#0c53af',
                strokeOpacity: 0.8,
                strokeWeight: 5,
            });
            line.setMap(map);
        }

    }
    map.fitBounds(bounds);
}


/////////Equipments//////////
intitUpdatesController();
function intitUpdatesController(){
    $(".throughput_edit").editable({
        mode: "inline",
        showbuttons: false,
        validate: function(value) {
            if($.trim(value) === '') {
                return 'Throughput is required';
            }
        }
    }).on('shown', function(e, editable) {
        editable.input.$input.attr("size", "1");
    }).on('save', function(e, params) {
        $(this).parents(".eq_group_item").children().eq(2).val(params.newValue);
    });
    $(".range_edit").editable({
        mode: "inline",
        showbuttons: false,
        validate: function(value) {
            if($.trim(value) === '') {
                return 'Range is required';
            }
        }
    }).on('shown', function(e, editable) {
        editable.input.$input.attr("size", "1");
    }).on('save', function(e, params) {
        $(this).parents(".eq_group_item").children().eq(3).val(params.newValue);
    });
    $(".cost_edit").editable({
        mode: "inline",
        showbuttons: false,
        validate: function(value) {
            if($.trim(value) === '') {
                return 'Cost is required';
            }
        }
    }).on('shown', function(e, editable) {
        editable.input.$input.attr("size", "1");
    }).on('save', function(e, params) {
        $(this).parents(".eq_group_item").children().eq(4).val(params.newValue);
    });
    ///
    $(".eq_group_item").click(function(){
        const allGraphEdges = g.edges(true);
        resetEdgesPropertiesInMap(allGraphEdges);
        console.log(allGraphEdges);
        if($(this).attr("eq-close") === "opened"){
            $(this).removeClass("active");
            $(this).attr("eq-close", "closed");
            $("#networkSummary li:last-child").remove();
            return;
        }
        $(".eq_group li").each(function(){
            $(this).removeClass("active");
            $(this).attr("eq-close","closed");
        });
        const equipmentId = $(this).attr("eq-id");
        $(this).addClass("active");
        $(this).attr("eq-close", "opened");
        let equipmentCount = 0;
        let singleEquipmentCost = 0;
        for(let i = 0; i < allGraphEdges.length; i++){
            if(allGraphEdges[i][2]["device_id"] === parseInt(equipmentId)){
                equipmentCount++;
                singleEquipmentCost = parseInt(allGraphEdges[i][2]["device_cost"]);
                allGraphEdges[i][2]["line"].setOptions({strokeOpacity: 0.7, strokeColor: '#af3114'});
            }
        }
        if(allGraphEdges.length !== 0){
            let newLi = "<li>Selected Equipment Used: <label class='badge'>"+equipmentCount+" * "+singleEquipmentCost+" = "+parseInt(equipmentCount) * parseInt(singleEquipmentCost)+"</label></li>";
            if($("#networkSummary").children().length === 5){
                $("#networkSummary").append(newLi);
            }else{
                $("#networkSummary li:last-child").remove();
                $("#networkSummary").append(newLi);
            }
        }

    });
}

$(".add_equipment_button").click(function(){
    if( !$(".newEquipmentForm")[0].checkValidity() ){
        alertify.notify('All fields are required', 'warning');
        return false;
    }
    let newId = parseInt($(".eq_group_item:last").attr("eq-id")) + 1;
    let newEquipmentHtml = '<li class="list-group-item eq_group_item" eq-close="closed" eq-id="'+newId+'">\n' +
        '                                        <b class="pull-left">'+$("#name").val()+'</b>\n' +
        '                                        <input class="pull-right device_select_check" name="equipment['+newId+'][id]" value="'+newId+'" checked="checked" type="checkbox">\n' +
        '                                        <input name="equipment['+newId+'][throughput]" value="'+$("#Throughput").val()+'" type="hidden">\n' +
        '                                        <input value="'+$("#name").val()+'" type="hidden">\n' +
        '                                        <input name="equipment['+newId+'][range]" value="'+$("#Range").val()+'" type="hidden">\n' +
        '                                        <input name="equipment['+newId+'][cost]" value="'+$("#Cost").val()+'" type="hidden">\n' +
        '                                        <br>\n' +
        '                                        <ul class="list-inline">\n' +
        '                                            <li><i><b><span class="throughput_edit">'+$("#Throughput").val()+'</span></b> MBits/s</i></li>\n' +
        '                                            <li><i><b><span class="range_edit">'+$("#Range").val()+'</span></b>KM</i></li>\n' +
        '                                            <li><i>$<b><span class="cost_edit">'+$("#Cost").val()+'</span></b></i></li>\n' +
        '                                        </ul>\n' +
        '                                    </li>';

    let equipmentObj = {
        id: newId.toString(),
        throughput: $("#Throughput").val(),
        name: $("#name").val(),
        range:$("#Range").val(),
        cost: $("#Cost").val()
    };
    allEquipments.push(equipmentObj);
    selectedEquipments.push(equipmentObj);
    console.log(allEquipments);
    $(".newEquipmentForm")[0].reset();
    $("#default_equipment_set").append(newEquipmentHtml);
    intitUpdatesController();
    updateSelectedEquipmentList();
    initilizeDeviceSelectList();
    $("#update_equipment_model").modal('hide');
});

updateSelectedEquipmentList();
function updateSelectedEquipmentList(){
    $(".device_select_check").click(function (){
        if($(this).prop('checked')){
            console.log("Selected: " + $(this).prev().html());
            let selectedId = $(this).val();
            for(let i = 0; i < allEquipments.length; i++){
                if(selectedId === allEquipments[i].id){
                    selectedEquipments.push(allEquipments[i]);
                    break;
                }
            }
            console.log("After New Selection");
            console.log(selectedEquipments);
            initilizeDeviceSelectList();
        }else{
            console.log("Un - Selected: " + $(this).prev().html());
            let selectedId = $(this).val();
            for(let i = 0; i < selectedEquipments.length; i++){
                if(selectedId === selectedEquipments[i].id){
                    selectedEquipments.splice(i, 1);
                    break;
                }
            }
            console.log("After Un - Selection");
            console.log(selectedEquipments);
            initilizeDeviceSelectList();
        }
    });
}

function initilizeDeviceSelectList(){
    let html = '<option value=""></option>';
    for (let i = 0; i < selectedEquipments.length; i++){
        html += '<option value="'+i+'">'+selectedEquipments[i].name+'</option>';
    }
    $("#associated_device_select").html(html);
}

function resetEdgesPropertiesInMap(allGraphEdges){
    for(let i = 0; i < allGraphEdges.length; i++){
        allGraphEdges[i][2]["line"].setOptions({strokeOpacity: 0.8, strokeColor: '#0c53af'});
    }
}
//////////Export/Import Map

$(".export_btn").click(function(){
    let id_attr = $(this).attr("id");
    switch (id_attr){
        case "export_img":
            html2canvas(document.querySelector(".gm-style"), {useCORS: true}).then(canvas => {
                let value = "ZyxtPlanning";
                let dataUrl= canvas.toDataURL("image/png");
                let imgBlob = dataURLtoBlob(dataUrl);
                let url = window.URL.createObjectURL(imgBlob);
                let a = document.createElement("a");
                document.body.appendChild(a);
                a.style = "display: none";
                a.href = url;
                a.download = value + ".png";
                a.click();
                window.URL.revokeObjectURL(url);
                document.body.removeChild(a);
            });
            break;
        case "export_zyxt":
            let  value = "ZyxtPlanning";
            let allNodes = g.nodes(true);
            let allEdges = g.edges(true);
            console.log(allNodes);
            console.log(allEdges);
            let mapData = {nodes:[], edges:[]};
            for(let i = 0; i < allNodes.length; i++){
                let node = {
                    graphNode: allNodes[i][0],
                    connected: allNodes[i][1]["marker"].connected,
                    id: allNodes[i][1]["marker"].id,
                    prop: allNodes[i][1]["prop"]
                };
                mapData["nodes"].push(node);
            }
            for(let i = 0; i < allEdges.length; i++) {
                let n1Prop = getNodePropertiesFromGraph(allEdges[i][0]); //finalResult[parseInt(allEdges[e][0])-1];
                let n2Prop = getNodePropertiesFromGraph(allEdges[i][1]); // finalResult[parseInt(allEdges[e][1])-1];
                let edge = {n1:allEdges[i][0],n2:allEdges[i][1],lat1: n1Prop.lat, lng1: n1Prop.lng, lat2: n2Prop.lat, lng2: n2Prop.lng,
                device: allEdges[i][2]['device']};
                mapData["edges"].push(edge);

            }
            console.log(mapData);
            let mapDataJson = JSON.stringify(mapData);
            let element = document.createElement('a');
            element.setAttribute('href', 'data:text/json;charset=utf-8,' + encodeURIComponent(mapDataJson));
            element.setAttribute('download', value + ".json");
            element.style.display = 'none';
            document.body.appendChild(element);
            element.click();
            document.body.removeChild(element);
            break;
    }
});

$("#zyxtFile").change(function(evt){
    let files = evt.target.files;
    //console.log(files);
    if (!files.length || files[0].size <= 0 || files[0].type !== "application/json") {
        alertify.error('Please select a valid json file!');
        $("#import_model").modal("hide");
        return;
    }
    let reader = new FileReader();
    reader.readAsText(files[0]);
    let bounds = new google.maps.LatLngBounds();
    reader.onload = function(evt){
        let mapDataObj = JSON.parse(evt.target.result);
        console.log(mapDataObj);
        for(let i = 0; i < mapDataObj["nodes"].length; i++){
            let node = mapDataObj["nodes"][i];
            bounds.extend(new google.maps.LatLng(node.prop.lat, node.prop.lng));
            let markerIcon = (node.prop.type === "sink") ? "/img/sink.png" : (node.prop.type === "source") ? "/img/source.png" : "/img/intermediate.png";
            marker = new google.maps.Marker({
                position: {lat: node.prop.lat, lng: node.prop.lng},
                title: node.prop.type,
                icon: markerIcon,
                id: markerCounter,
                draggable:(!node.connected),
                connected: node.connected,
                graphNode: node.graphNode
            });
            marker.setMap(map);
            g.addNode(node.graphNode, {prop:node.prop,marker:marker});
            if(node.prop.type !== "intermediate"){
                let mountingHeight = (parseInt(node.prop.mountingHeight) === 0) ? defaultMountingHeight : parseInt(node.prop.mountingHeight);
                let nodeCapacity = (parseInt(node.prop.capacity) < 1) ? parseInt(node.prop.capacity) * -1 : parseInt(node.prop.capacity);
                let infowindow = new google.maps.InfoWindow({
                    content: setInfoWindowContent(nodeCapacity, mountingHeight, node.prop.type)
                });
                let resObj = {marker: marker,infoWindow: infowindow, mounting:mountingHeight, capacity:nodeCapacity,type:node.prop.type};
                inputJson[markerCounter] = resObj;
                markerCounter++;
                initializeMarker(marker);
            }

        }
        for(let i = 0; i < mapDataObj["edges"].length; i++){
            let edge = mapDataObj["edges"][i];
            let tail = {lat:  parseFloat(edge.lat1), lng: parseFloat(edge.lng1) };
            let head = {lat:  parseFloat(edge.lat2), lng: parseFloat(edge.lng2) };
            let line = new google.maps.Polyline({
                path: [tail,head],
                geodesic: true,
                strokeColor: '#0c53af',
                strokeOpacity: 0.8,
                strokeWeight: 5,
                from: edge.n1,
                to: edge.n2
            });
            g.addEdge(edge.n1, edge.n2, {line:line, device: edge.device});
            line.setMap(map);
            google.maps.event.addListener(line, 'click', function(event) {
                let edge = getEdgeFromGrpah(line.from, line.to);
                console.log(edge);
                $("#associated_device_select").prev().prev().val(line.from);
                $("#associated_device_select").prev().val(line.to);
                $("#associated_device_select").val(getIndexOfSelectEquipment(edge[2]['device']['id']));
                initDeviceMetaData(getIndexOfSelectEquipment(edge[2]['device']['id']));
                $("#associate_device_modal").modal("show");
            });
        }
        $("#import_model").modal("hide");
        map.fitBounds(bounds);
        console.log(markerCounter);
        console.log(inputJson);
        console.log("Nodes\n");
        console.log(g.nodes(true));
        console.log("Edges\n");
        console.log(g.edges(true));
    }
});

$("#associated_device_select").change(function () {
    initDeviceMetaData($(this).val());
});

function initDeviceMetaData(deviceID, edgeProperty){
    $("#output_sel_dev_section").addClass("hide");
    let device = selectedEquipments[deviceID];
    if(device !== undefined) {
        $("#sel_device_throughput").val(device.throughput + ' MBits / s');
        $("#sel_device_range").val(device.range + ' Km');
        $("#sel_device_cost").val('$'+device.cost);
    }else{
        $("#sel_device_throughput").val('');
        $("#sel_device_range").val('');
        $("#sel_device_cost").val('');
    }
    if(edgeProperty !== undefined){
        console.log(edgeProperty);
        $("#output_sel_dev_section").removeClass("hide");
        $("#sel_device_passed_flow").val(edgeProperty.flowPassed + " MBits / s");
        $("#sel_device_edge_length").val(edgeProperty.length + " Km")
    }
}

function dataURLtoBlob(dataurl) {
    var arr = dataurl.split(','), mime = arr[0].match(/:(.*?);/)[1],
        bstr = atob(arr[1]), n = bstr.length, u8arr = new Uint8Array(n);
    while(n--){
        u8arr[n] = bstr.charCodeAt(n);
    }
    return new Blob([u8arr], {type:mime});
}
