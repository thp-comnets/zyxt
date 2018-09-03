package com.nyu.zyxt.Services;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;

import javax.annotation.PostConstruct;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.util.concurrent.BlockingQueue;

@Service
public class ZyxtJobRunner {

    private BlockingQueue<ZyxtJobHandler> queue;
    @Value("${mainOutputDir}")
    private String mainOutputDir;
    private Logger logger = LoggerFactory.getLogger(this.getClass());

    public void setQueue(BlockingQueue<ZyxtJobHandler> queue) {
        this.queue = queue;
    }

    @PostConstruct
    public void init(){
        logger.info("Init ZyxtJobRunner Runtime");
    }

    @Async
    public void run() {
        try{
            while (true) {
                logger.info("In Runner, Waiting For The Job");
                ZyxtJobHandler zyxtJobHandler = this.queue.take();
                String command = zyxtJobHandler.getCommand();
                logger.info("In Runner, New Job Received: " + command);
                ProcessBuilder processBuilder = new ProcessBuilder();
                processBuilder.command("sh", "-c", command);
                processBuilder.directory(new File(System.getProperty("user.home")));
                new File(this.mainOutputDir + "/newInputMap_"+zyxtJobHandler.getJobId()).mkdir();
                File errorFile = new File(this.mainOutputDir + "/newInputMap_"+zyxtJobHandler.getJobId()+"/error.txt");
                errorFile.createNewFile();
                File outputFile = new File(this.mainOutputDir + "/newInputMap_"+zyxtJobHandler.getJobId()+"/output.txt");
                outputFile.createNewFile();
                processBuilder.redirectError(errorFile);
                processBuilder.redirectOutput(outputFile);
                Process process = processBuilder.start();
                zyxtJobHandler.setJobProcess(process);
                Field f = process.getClass().getDeclaredField("pid");
                f.setAccessible(true);
                long pid = f.getLong(process);
                f.setAccessible(false);
                logger.info("PID is: " + Long.toString(pid));
                int exitCode = process.waitFor();
                logger.info("Map Processing Done With Exit Value: " + exitCode + "....");
            }
        }catch (Exception ex){
            logger.error("This is error in runner");
            logger.error(ex.getMessage());
        }
    }
}
