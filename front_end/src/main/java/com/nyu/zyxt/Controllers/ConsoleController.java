package com.nyu.zyxt.Controllers;

import com.nyu.zyxt.Services.ZyxtService;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.*;

import java.io.IOException;

@Controller
public class ConsoleController {

    @Autowired
    ZyxtService zyxtService;
    private Logger logger = LoggerFactory.getLogger(this.getClass());

    @RequestMapping(value = "/admin")
    public String consoleHome(Model model) throws IOException {
        model.addAttribute("users", zyxtService.getAllUsers());
        return "console";
    }

    @PostMapping(value = "/admin/delete_job/{job_id}/{user_id}")
    @ResponseBody
    public boolean deleteJob(@PathVariable String job_id, @PathVariable String user_id) throws IOException {
        boolean res;
        if(zyxtService.checkValidityOfJob(job_id).equals("TRUE")){
            res = this.zyxtService.deleteJob(user_id, job_id);
        }else{
            try{
                res = this.zyxtService.cancelAndDeleteJob(job_id, user_id);
            }catch(Exception ex){
                res = false;
                logger.info("Error in cancelling: " + ex.getMessage());
            }
        }
        return res;
    }

}
