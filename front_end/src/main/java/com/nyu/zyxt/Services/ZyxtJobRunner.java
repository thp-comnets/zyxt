package com.nyu.zyxt.Services;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.scheduling.annotation.Async;
import org.springframework.stereotype.Service;

import javax.annotation.PostConstruct;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.lang.reflect.Field;
import java.util.concurrent.BlockingQueue;

@Service
public class ZyxtJobRunner {

    private BlockingQueue<ZyxtJobHandler> queue;
    private Runtime runtime;
    private Process process;
    private BufferedReader bufferedReader;
    private String line;
    private Logger logger = LoggerFactory.getLogger(this.getClass());

    public void setQueue(BlockingQueue<ZyxtJobHandler> queue) {
        this.queue = queue;
    }

    @PostConstruct
    public void init(){
        logger.info("Init ZyxtJobRunner Runtime");
        this.runtime = Runtime.getRuntime();
    }

    @Async
    public void run() {
        try{
            while (true) {
                logger.info("In Runner, Waiting For The Job");
                ZyxtJobHandler zyxtJobHandler = this.queue.take();
                String command = zyxtJobHandler.getCommand();
                logger.info("In Runner, New Job Received: " + command);
                String[] cmd = {
                        "/bin/bash",
                        "-c",
                        command
                };
                this.process = this.runtime.exec(cmd);
                logger.info("After Execution Of Script");
                zyxtJobHandler.setJobProcess(this.process);
                /********PID************/
                Field f = this.process.getClass().getDeclaredField("pid");
                f.setAccessible(true);
                long pid = f.getLong(this.process);
                f.setAccessible(false);

                /**********************/
                logger.info("PID is: " + Long.toString(pid));
                this.bufferedReader = new BufferedReader(new InputStreamReader(this.process.getInputStream()));
                while ((line = this.bufferedReader.readLine()) != null) {
                    System.out.println(line);
                }
                System.out.println("After End Of Output");
                System.out.flush();
                this.process.waitFor();
                logger.info("Map Processing Done With Exit Value: " + Integer.toString(this.process.exitValue()) + "....");
            }
        }catch (Exception ex){
            logger.error(ex.getMessage());
        }

//        String command = "python "+zyxtDir+"/processMapsWithRescaling.py " + zyxtDir + "/customerNodes/sampleMapSinks.csv " + zyxtDir + "/customerNodes/sampleMapSource.csv 400 n 0 fcnf /home/cuda/Dropbox/wisp_planning_tool/zyxt";
//        System.out.println(command);
    }
}
