//
// Created by 孙宇 on 2020/7/28.
//
/***********************************************************************************************
 ***                                Y S H E N - S T U D I O S                                ***
 ***********************************************************************************************
                                                                                              
                  Project Name : CppLearningDemos 
                                                                                              
                     File Name : servers 
                                                                                              
                    Programmer : 孙宇 
                                                                                              
                    Start Date : 2020/7/28 
                                                                                              
                   Last Update : 2020/7/28 
                                                                                              
 *---------------------------------------------------------------------------------------------*
  Description:           
        各Servers示例学习
  
 *---------------------------------------------------------------------------------------------*
  Functions:
 *---------------------------------------------------------------------------------------------*
  Updates:
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <iostream>
#include <boost/thread/xtime.hpp>

int main() {
    //计时
    auto start_time = boost::get_system_time();
    std::cout << "Ready, GO!" << std::endl << std::endl;



    long end_time = (boost::get_system_time() - start_time).total_milliseconds();
    std::cout << "Completed in: " << end_time << "ms" << std::endl;
    return 0;
}