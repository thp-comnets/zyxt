
#Zyxt - WISP Planning Tool

## How to run Zyxt

### Resources Required
- [GRASS GIS](https://grass.osgeo.org/ "GRASS GIS") should be successfully installed on the machine
- [Google OR-Tools](https://developers.google.com/optimization/)
- NVIDIA GPU

### Before getting started:
- .csv file containing the locations of customer
 - Format of the customer input file: *lat,lng,demand,revenue_from_customer*
-  .csv file containing the source (only one supported at the moment)
 - Format of the source input file: *lat,lng*

###### You can find some exmaples in **/customerNodes** directory
------------

### There are two scripts available to process your map.
> - **processMapsWithRescaling.py**  (Generally for big maps)
- **processMapWithOriginalResolution.py**

##### Parameteres required to run *processMapsWithRescaling.py*
1. Customer Input File Name
2. Source Input File Name
3. Size, you want to rescale the input map.
4. Node Removal Algorithm
	c => Clustering Removal
	r => Random Removal
	e => Elevation Removal
	n => No removal
5. Removal % if removal algorithm is applied
6. Solver Algorithm
	 *fcnf* for **Fixed-Charge Network Flow**
	 *mcf* for ** Min Cost Flow With Randomized Rounding Technique**

**Example:**
```bash
python processMapsWithRescaling.py mapA_sinks.csv mapA_source.csv 400 n 0 fcnf
```

##### Parameteres required to run *processMapWithOriginalResolution.py*
1. Customer Input File Name
2. Source Input File Name
3. Node Removal Algorithm
	c => Clustering Removal
	r => Random Removal
	e => Elevation Removal
	n => No removal
4. Removal % if removal algorithm is applied
5. Solver Algorithm
	 *fcnf* for **Fixed-Charge Network Flow**
	 *mcf* for ** Min Cost Flow With Randomized Rounding Technique**

**Example:**
```bash
python processMapWithOriginalResolution.py mapA_sinks.csv mapA_source.csv c 99.95 mcf
```

------------


<span style="color:red">Please make sure to put the customer and source input file in <b>/customerNodes</b> directory</span>

