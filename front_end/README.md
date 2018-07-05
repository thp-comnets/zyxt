#Zyxt - WISP Planning Tool (Front End)

### Resources Required
- JAVA 8
- MAVEN

_Note: If you run ****zyxt**** from front end then just install the following dependent software/hardware:_<br/>
- [GRASS GIS](https://grass.osgeo.org/ "GRASS GIS") should be successfully installed on the machine
- [Google OR-Tools](https://developers.google.com/optimization/)
- NVIDIA GPU


### How to run Zyxt Front End

- First open the file _front_end/src/main/resources/application.properties_ file and
 modify the following properties according to your system.<br/>
    * server.port => Mention free port number for zyxt front end application.
    * mainOutputDir => Directory for seving outputs
    * mainZyxtDir => Directory of zyxt backend

- Now point the terminal to ****front_end**** directory and run following command

```bash
mvn clean install
````
After this, you will find the fat ****_.jar_**** file in _/target_ folder. Run this jar
file using the following command.

````bash
java -jar *.jar
````
At this point you can access the front end at the URL _http://localhost:[your_mentioned_port]_





