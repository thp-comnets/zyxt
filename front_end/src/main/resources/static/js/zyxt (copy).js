var map;
var markerType = -1;
var marker = null;
var markerCounter = 0;
var currentPosLatLng;
var updatingMarker = null;
var removingMarker = null;
$(".sourceBtn").click(function(){
    markerType = 0;
    makeBtnSelect($(this));
    makeBtnDefault($(".sinkBtn"));
});
$(".sinkBtn").click(function(){
    markerType = 1;
    makeBtnSelect($(this));
    makeBtnDefault($(".sourceBtn"));
});

function makeBtnSelect(btn){
    btn.removeClass("btn-primary");
    btn.addClass("btn-success");
}

function makeBtnDefault(btn){
    btn.removeClass("btn-success");
    btn.addClass("btn-primary");
}

var inputJson = {};
function initMap() {
    map = new google.maps.Map(document.getElementById('map'), {
        center: {lat: 38.4615244, lng: -122.93603567},
        scaleControl: true,
        mapTypeId: 'terrain',
        zoom: 8
    });
    map.addListener('click', function(event) {
        $(".save_btn").html("Create");
        $(".remove_btn").addClass("hide");
        if(markerType == 0){
            if(isSourceExist()){
                alertify.set('notifier','position', 'top-right');
                alertify.warning("Only One Source Supported");
                markerType = -1;
                return false;
            }
            $(".modal-title").html("Source");
            currentPosLatLng = event.latLng;
            $("#cap_modal").modal("show");

        }else if(markerType == 1){
            $(".modal-title").html("Sink");
            currentPosLatLng = event.latLng;
            $("#cap_modal").modal("show");
        }

    });
    $('#cap_modal').on('hidden.bs.modal', function (e) {
        if(removingMarker !== null){
            console.log("Removing Marker");
            var markerObj = inputJson[removingMarker];
            markerObj.marker.setMap(null);
            delete inputJson[removingMarker];
        }else if(updatingMarker !== null){
            console.log("Updating Marker");
            var markerObj = inputJson[updatingMarker];
            markerObj.capacity = $("#inputCapacity").val();
            markerObj.mounting = $("#mountingHeight").val();
            var type = markerObj.type;
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent($("#inputCapacity").val(), $("#mountingHeight").val(), type)
            });
            markerObj.infoWindow = infowindow;
        }else if(markerType === 0){
            console.log("Creating Source");
            marker = new google.maps.Marker({
                position: currentPosLatLng,
                title:"Source",
                icon: "/img/source.png",
                id: markerCounter,
                draggable:true
            });
            marker.setMap(map);
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent($("#inputCapacity").val(), $("#mountingHeight").val(), "source")
            });
            var resObj = {marker: marker, infoWindow: infowindow, mounting:$("#mountingHeight").val(), capacity:$("#inputCapacity").val(),type:"source"};
            inputJson[markerCounter] = resObj;
            markerCounter++;
            initializeMarker(marker);
        }else if(markerType === 1) {
            console.log("Creating Sink");
            marker = new google.maps.Marker({
                position: currentPosLatLng,
                title:"Sink",
                icon: "/img/sink.png",
                id: markerCounter,
                draggable:true
            });
            marker.setMap(map);
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent($("#inputCapacity").val(), $("#mountingHeight").val(), "sink")
            });
            var resObj = {marker: marker,infoWindow: infowindow, mounting:$("#mountingHeight").val(), capacity:$("#inputCapacity").val(),type:"sink"};
            inputJson[markerCounter] = resObj;
            markerCounter++;
            initializeMarker(marker);
        }
        console.log(inputJson);
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
        var bounds = new google.maps.LatLngBounds();
        if($('.sinks').val() !== ""){
            var sinksLocations = $('.sinks').val().split(/\n/);
            var totalSourceCapacity = 0;
            for(var i = 0; i < sinksLocations.length; i++){
                var locData = sinksLocations[i].split(",");
                console.log("Creating Sink");
                var sinkPos = new google.maps.LatLng(parseFloat(locData[0]), parseFloat(locData[1]));
                bounds.extend(sinkPos);
                marker = new google.maps.Marker({
                    position: sinkPos,
                    title:"Sink",
                    icon: "/img/sink.png",
                    id: markerCounter,
                    draggable:true
                });
                marker.setMap(map);
                var infowindow = new google.maps.InfoWindow({
                    content: setInfoWindowContent(parseInt(locData[2]), null, "sink")
                });
                totalSourceCapacity += parseInt(locData[2]);
                var resObj = {marker: marker,infoWindow: infowindow, mounting:$("#mountingHeight").val(), capacity:parseInt(locData[2]),type:"sink"};
                inputJson[markerCounter] = resObj;
                markerCounter++;
                initializeMarker(marker);
            }
        }
        if($(".source").val() !== ""){
            var sourceLocation = $(".source").val().split(",");
            console.log("Creating Source");
            var sourcePos = new google.maps.LatLng(parseFloat(sourceLocation[0]), parseFloat(sourceLocation[1]));
            bounds.extend(sourcePos);
            marker = new google.maps.Marker({
                position: sourcePos,
                title:"Source",
                icon: "/img/source.png",
                id: markerCounter,
                draggable:true
            });
            marker.setMap(map);
            var infowindow = new google.maps.InfoWindow({
                content: setInfoWindowContent(totalSourceCapacity, $("#mountingHeight").val(), "source")
            });
            var resObj = {marker: marker, infoWindow: infowindow, mounting:$("#mountingHeight").val(), capacity:totalSourceCapacity,type:"source"};
            inputJson[markerCounter] = resObj;
            markerCounter++;
            initializeMarker(marker);
        }

        $(".sinks").val("");
        $(".source").val("");
        $("#location_data").modal("hide");
        map.fitBounds(bounds);
        console.log(inputJson);
    });

}

function initializeMarker(marker){
    $(".save_btn").html("Update");
    google.maps.event.addListener(marker, 'click', function(event) {
        $(".remove_btn").removeClass("hide");
        var markerObj = inputJson[marker.id];
        $("#inputCapacity").val(markerObj.capacity);
        $("#mountingHeight").val(markerObj.mounting);
        $(".modal-title").html(markerObj.type);
        updatingMarker = marker.id;
        $("#cap_modal").modal("show");
    });
    google.maps.event.addListener(marker, 'mouseover', function(event) {
        var markerObj = inputJson[marker.id];
        markerObj.infoWindow.open(map, marker);
    });
    google.maps.event.addListener(marker, 'mouseout', function(event) {
        var markerObj = inputJson[marker.id];
        markerObj.infoWindow.close();
    });
}

function setInfoWindowContent(capacity, mountingHeight, type) {
    var content = '';
    var className = (type === "source") ? "text-danger" : "text-success";
    content += '<ul>Type: <b class="'+className+'">'+type+'</b></ul>';

    content += '<ul> Capacity: ' + capacity + ' Mbit/s </ul>';

    // content += '<ul> Mounting Height: ' + mountingHeight + ' m </ul>';


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


function showOutput(outputJson){
    var obj = jQuery.parseJSON( outputJson );
    var html = "Total Network Cost: <b class='badge'>$"+obj.equipmentCost+"</b>"
    $("#output").html(html);
    var processNodes = {};

    var bounds = new google.maps.LatLngBounds();
    for(var i = 0; i < obj.edges.length; i++){
        // console.log(obj.edges[i].nodes[0] + " => " + obj.edges[i].nodes[1]);
        if(processNodes[ obj.edges[i].nodes[0] ] === undefined){
            var nodeProp = getPropertiesOfNode(obj.edges[i].nodes[0], obj);
            bounds.extend(new google.maps.LatLng(nodeProp.lat, nodeProp.lng));
            var markerIcon = (nodeProp.type === "sink") ? "/img/sink.png" : (nodeProp.type === "source") ? "/img/source.png" : "/img/intermediate.png";
            console.log(obj.edges[i].nodes[0]);
            marker = new google.maps.Marker({
                position: {lat: nodeProp.lat, lng: nodeProp.lng},
                //title: nodeProp.type,
                icon: markerIcon
            });
            marker.setMap(map);
            processNodes[ obj.edges[i].nodes[0] ] = nodeProp;
        }
        if(!processNodes[ obj.edges[i].nodes[1] ]){
            var nodeProp = getPropertiesOfNode(obj.edges[i].nodes[1], obj);
            bounds.extend(new google.maps.LatLng(nodeProp.lat, nodeProp.lng));
            var markerIcon = (nodeProp.type === "sink") ? "/img/sink.png" : (nodeProp.type === "source") ? "/img/source.png" : "/img/intermediate.png";
            console.log(obj.edges[i].nodes[1]);
            marker = new google.maps.Marker({
                position: {lat: nodeProp.lat, lng: nodeProp.lng},
                //title: nodeProp.type,
                icon: markerIcon
            });
            marker.setMap(map);
            processNodes[ obj.edges[i].nodes[1] ] = nodeProp;
        }
        var tail = {lat:  processNodes[ obj.edges[i].nodes[0] ].lat, lng:processNodes[ obj.edges[i].nodes[0] ].lng };
        var head = { lat:  processNodes[ obj.edges[i].nodes[1] ].lat, lng:processNodes[ obj.edges[i].nodes[1] ].lng };
        var line = new google.maps.Polyline({
            path: [tail,head],
            geodesic: true,
            strokeColor: '#0c53af',
            strokeOpacity: 0.8,
            strokeWeight: 5,
            clickable: true
        });
        line.infoWindow = new google.maps.InfoWindow({
            content: setInfoWindowContentForEdge(obj.edges[i].edgeProperty)
        });
        google.maps.event.addListener(line, 'click', function(event) {
            this.infoWindow.open(map)
            this.infoWindow.setPosition(event.latLng)
        });
        line.setMap(map);

    }
    map.fitBounds(bounds);
    //map.setZoom(11);

}