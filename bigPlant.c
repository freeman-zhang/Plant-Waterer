#include "gpiolib_addr.h"
#include "gpiolib_reg.h"

#include <stdint.h>
#include <stdio.h>		//for the printf() function
#include <fcntl.h>
#include <linux/watchdog.h> 	//needed for the watchdog specific constants
#include <unistd.h> 		//needed for sleep
#include <sys/ioctl.h> 		//needed for the ioctl function
#include <stdlib.h> 		//for atoi
#include <time.h> 		//for time_t and the time() function
#include <sys/time.h>           //for gettimeofday()

//macro for writing log message
//file        - will be the file pointer to the log file
//time        - will be the current time at which the message is being printed
//programName - will be the name of the program, in this case it will be Lab4Sample
//str         - will be a string that contains the message that will be printed to the file.
#define PRINT_MSG(file, time, programName, str) \
	do{ \
			fprintf(logFile, "%s : %s : %s", time, programName, str); \
			fflush(logFile); \
	}while(0)
		
	
//This function should initialize the GPIO pins
GPIO_Handle initializeGPIO()
{
	GPIO_Handle gpio;
	gpio = gpiolib_init_gpio();
	if(gpio == NULL){
		perror("Could not initialize GPIO");
	}
	return gpio;

}	


void setToOutput(GPIO_Handle gpio, int pinNumber){
	
	//Check that the gpio is functional
	if(gpio == NULL)
	{
		printf("The GPIO has not been intitialized properly \n");
		return;
	}

	//Check that we are trying to set a valid pin number
	if(pinNumber < 2 || pinNumber > 27)
	{
		printf("Not a valid pinNumer \n");
		return;
	}
	int registerNum = pinNumber / 10;
	int bitShift = (pinNumber % 10) * 3;
	uint32_t sel_reg = gpiolib_read_reg(gpio, GPFSEL(registerNum));
	sel_reg |= 1  << bitShift;
	gpiolib_write_reg(gpio, GPFSEL(1), sel_reg);
}

//Checking the status of the moisture sensor
#define SENSOR_PIN 4
int getMoisture(GPIO_Handle gpio){
	if (gpio == NULL){
		return -1;
	}
	
	uint32_t level_reg = gpiolib_read_reg(gpio, GPLEV(0));
	return level_reg & (1 << SENSOR_PIN);
}


#define MOTOR_PIN 17
//turns the motor On
void turnMotorOn(GPIO_Handle gpio){
	//set the motor pin to output pin
	setToOutput(gpio, MOTOR_PIN);
	gpiolib_write_reg(gpio, GPSET(0), 1 << MOTOR_PIN);
}

//turns the motor off
void turnMotorOff(GPIO_Handle gpio){
	//set the motor pin to the output pin
	setToOutput(gpio, MOTOR_PIN);
	gpiolib_write_reg(gpio, GPCLR(0), 1 << MOTOR_PIN);
}

//Robust readConfig using state machine
void readConfig(FILE* configFile, int* timeout, char* logFileName, char* statFileName)
{
	//Loop counter
	int i = 0;
	
	//A char array to act as a buffer for the file
	char buffer[255];

	//The value of the timeout variable is set to fifteen at the start
	*timeout = 0;
    
    //States
    enum State { START, WATCHDOG, LOGFILE, STATFILE, DONE };
    
    enum State s = START;

	//This will 
	//fgets(buffer, 255, configFile);
	//This will check that the file can still be read from and if it can,
	//then the loop will check to see if the line may have any useful 
	//information.
	while(fgets(buffer, 255, configFile) != NULL)
	{
        s = START;
        // Index of the character array "buffer"
		i = 0;
        
        //////////////////////////////////////////////
        //
        // Analyze first letter of the line
        //
        switch(s)
        {
            
            case START:
                
                // Check the first letter of each line
                // If it's not a comment, see if it's a significant letter
                if(buffer[i] == 'W')
                {
                    s = WATCHDOG;
                }
                else if(buffer[i] == 'L')
                {
                    s = LOGFILE;
                }
                else if(buffer[i] == 'S')
                {
                    s = STATFILE;
                }
                
                // If not a specific letter, line doesn't contain useful information
                else
                {
                    s = DONE;
                }
                
                break;

            case WATCHDOG:
                break;
            case LOGFILE:
                break;
            case STATFILE:
                break;
            case DONE:
                break;
                
        }
        
        ////////////////////////////////////////////////////
        //
        // Analyze the contents of the line for an '=' sign
        //
        
        switch(s)
        {
            
            case START:
                break;
            
            case WATCHDOG:
                
                // Default to done if we don't find an '=' sign in the line
                s = DONE;
                
                // While we have not encountered a comment or end of the line
                while(buffer[i] != '#' && buffer[i] != '\n' && buffer[i] != 0){
                    
                    // If we encounter the '=' sign, now look for the value
                    if(buffer[i] == '=')
                    {
                        s = WATCHDOG;
                        break;
                    }
                    
                    i++;
                }
                
                break;
                
            case LOGFILE:
                
                // Default to done if we don't find an '=' sign in the line
                s = DONE;
                
                // While we have not encountered a comment or end of the line
                while(buffer[i] != '#' && buffer[i] != '\n' && buffer[i] != 0){
                    
                    // If we encounter the '=' sign, now look for the value
                    if(buffer[i] == '=')
                    {
                        s = LOGFILE;
                        break;
                    }
                    
                    i++;
                }
                
                break;
                
            case STATFILE:
                
                // Default to done if we don't find an '=' sign in the line
                s = DONE;
                
                // While we have not encountered a comment or end of the line
                while(buffer[i] != '#' && buffer[i] != '\n' && buffer[i] != 0){
                    
                    // If we encounter the '=' sign, now look for the value
                    if(buffer[i] == '=')
                    {
                        s = STATFILE;
                        break;
                    }
                    
                    i++;
                }
                
                break;

            case DONE:
                break;
                
        }
        
        //////////////////////////////////////////////////
        //
        // Get the values in the significant lines
        //
        
        switch(s){
            
            case START:
                break;
            
            case WATCHDOG:
                
                // While there isn't a comment or end of the line
                while(buffer[i] != '#' && buffer[i] != 0)
                {
                    // If the character is a digit
                    if(buffer[i] >= '0' && buffer[i] <= '9')
                    {
                        // Calculate the value
                        *timeout = (*timeout *10) + (buffer[i] - '0');
                    }
                    
                    i++;
                }
                
                s = DONE;
                
                break;
                
            case LOGFILE:
			{
                int j = 0;
                //Loop runs while the character is not a newline or null
                while(buffer[i] != 0  && buffer[i] != '\n' && buffer[i] != '#')
                {
                    //If the characters after the equal sign are not spaces or
                    //equal signs, then it will add that character to the string
                    if(buffer[i] != ' ' && buffer[i] != '=')
                    {
                        logFileName[j] = buffer[i];
                        j++;
                    }
                    i++;
                }
                //Add a null terminator at the end
                logFileName[j] = 0;
                
                s = DONE;
                
                break;
			}
            case STATFILE:
			{
                int k = 0;
                //Loop runs while the character is not a newline or null
                while(buffer[i] != 0  && buffer[i] != '\n' && buffer[i] != '#')
                {
                    //If the characters after the equal sign are not spaces or
                    //equal signs, then it will add that character to the string
                    if(buffer[i] != ' ' && buffer[i] != '=')
                    {
                        statFileName[k] = buffer[i];
                        k++;
                    }
                    i++;
                }
                //Add a null terminator at the end
                statFileName[k] = 0;
                
                s = DONE;
                
                break;
			}
            case DONE:
                break;
        }
    }
}

//This function will get the current time using the gettimeofday function
void getTime(char* buffer)
{
	//Create a timeval struct named tv
  	struct timeval tv;

	//Create a time_t variable named curtime
  	time_t curtime;


	//Get the current time and store it in the tv struct
  	gettimeofday(&tv, NULL); 

	//Set curtime to be equal to the number of seconds in tv
  	curtime=tv.tv_sec;

	//This will set buffer to be equal to a string that in
	//equivalent to the current date, in a month, day, year and
	//the current time in 24 hour notation.
  	strftime(buffer,30,"%m-%d-%Y  %T.",localtime(&curtime));

} 

int main(const int argc, const char* const argv[]){
	
	//Create a string that contains the program name
	const char* argName = argv[0];
	
		//These variables will be used to count how long the name of the program is
	int i = 0;
	int namelength = 0;

	while(argName[i] != 0)
	{
		namelength++;
		i++;
	} 
	
	char programName[namelength];
	
	i = 0;

	//Copy the name of the program without the ./ at the start
	//of argv[0]
	while(argName[i + 2] != 0)
	{
		programName[i] = argName[i + 2];
		i++;
	} 	
	
	//Create a file pointer named configFile
	FILE* configFile;
	//Set configFile to point to the Lab4Sample.cfg file. It is
	//set to read the file.
	configFile = fopen("/home/pi/plant.cfg", "r");
	
	//Output a warning message if the file cannot be openned
	if(!configFile)
	{
		perror("The config file could not be opened");
		return -1;
	}
	
	//declare the variables passed from readConfig function
	int timeout;
	char logFileName[50];
	char statFileName[50];
	
	//Call the readConfig function to read from the config file
	readConfig(configFile, &timeout, logFileName, statFileName);
	
	//Close the configFile now that we have finished reading from it
	fclose(configFile);

	//Create a new file pointer to point to the log file
	FILE* logFile;
	//Set it to point to the file from the config file and make it append to
	//the file when it writes to it.
	logFile = fopen(logFileName, "a");
	
	//Check that the file exists. If not, create a new one
	if(!logFile){
		logFile = fopen(logFileName, "wb");
		PRINT_MSG(logFile, time, programName, "Log file could not be found, created a new one\n\n");
	}
	
	//creates a new file pointer to point the stat file
	FILE* statFile;
	//set it to point to the file from the config file and make it append to 
	//the file when it writes to it.
	statFile = fopen(statFileName, "a");
	
	//checks that file exists. If not, create a new one
	if (!statFile){
		statFile = fopen(statFileName, "wb");
		PRINT_MSG(logFile, time, programName, "Stat file could not be found, created a new one\n\n");
	}
	
	//Create a char array that will be used to hold the time values
	char time[30];
	getTime(time);
	
	//Initialize the GPIO pins
	GPIO_Handle gpio = initializeGPIO();
	
	//Get the current time
	getTime(time);
	//Log that the GPIO pins have been initialized
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been initialized\n\n");
	
	//Set pin 17 of the GPIO pins to an output
	setToOutput(gpio, 17);
	//Get the current time
	getTime(time);
	//Log that pin 17 has been set to an output
	PRINT_MSG(logFile, time, programName, "Pin 17 has been set to output\n\n");
	
	//This variable will be used to access the /dev/watchdog file, similar to how
	//the GPIO_Handle works
	int watchdog;

	//We use the open function here to open the /dev/watchdog file. If it does
	//not open, then we output an error message. We do not use fopen() because we
	//do not want to create a file if it doesn't exist
	if ((watchdog = open("/dev/watchdog", O_RDWR | O_NOCTTY)) < 0) {
		printf("Error: Couldn't open watchdog device! %d\n", watchdog);
		return -1;
	} 
	
	//Get the current time
	getTime(time);
	//Log that the watchdog file has been opened
	PRINT_MSG(logFile, time, programName, "The Watchdog file has been opened\n\n");
	
	ioctl(watchdog, WDIOC_SETTIMEOUT, &timeout);
	
	//Get the current time
	getTime(time);
	//Log that the Watchdog time limit has been set
	PRINT_MSG(logFile, time, programName, "The Watchdog time limit has been set\n\n");
	
	//declaring enum States
	enum State{START, DRY, WET, DONE};
	enum State s = START;
	
	//Get the current time
	getTime(time);
	//Log the change in state
	PRINT_MSG(logFile, time, programName, "State has been set to START\n\n");
	
	
	
	while(s != DONE){
		int isDry = getMoisture(gpio);
		clock_t startPumpTime;
		switch(s){
			case START:
				s = WET;
				//Get the current time
				getTime(time);
				//Log the change in state
				PRINT_MSG(logFile, time, programName, "State has been set to WET\n\n");
				break;

			case DRY:
				turnMotorOn(gpio);
				
				if (!isDry){
					s = WET;
					//Get the current time
					getTime(time);
					//Log the change in state
					PRINT_MSG(logFile, time, programName, "State has been set to WET\n\n");
					//calculation of water pumped based on time running
					//Rough approximation with pumping speed obtained from manufacturer
					clock_t timePassed = clock() - startPumpTime;
					float pumpTime = timePassed/CLOCKS_PER_SEC;
					int waterPumped = pumpTime * 15;
					PRINT_MSG(statFile, time, programName, waterPumped);
					PRINT_MSG(statFile, time, programName, " mL of water was pumped\n\n");
				}			
				break;
				
			case WET:
				turnMotorOff(gpio);
				
				if (isDry){
					s = DRY;
					//Get the current time
					getTime(time);
					//Log the change in state
					PRINT_MSG(logFile, time, programName, "State has been set to DRY\n\n");
					startPumpTime = clock();
				}			
				break;
				
			case DONE:
				break;
				
			default:
				break;
		}
		//This ioctl call will write to the watchdog file and prevent 
		//the system from rebooting. It does this every 2 seconds, so 
		//setting the watchdog timer lower than this will cause the timer
		//to reset the Pi after 1 second
		ioctl(watchdog, WDIOC_KEEPALIVE, 0);
		getTime(time);
		//Log that the Watchdog was kicked
		PRINT_MSG(logFile, time, programName, "The Watchdog was kicked\n\n");
		//delay by 1 second
		usleep(100000);
	}
	
	//Writing a V to the watchdog file will disable to watchdog and prevent it from
	//resetting the system
	write(watchdog, "V", 1);
	getTime(time);
	//Log that the Watchdog was disabled
	PRINT_MSG(logFile, time, programName, "The Watchdog was disabled\n\n");
	
	//Close the watchdog file so that it is not accidentally tampered with
	close(watchdog);
	getTime(time);
	//Log that the Watchdog was closed
	PRINT_MSG(logFile, time, programName, "The Watchdog was closed\n\n");

	//Free the gpio pins
	gpiolib_free_gpio(gpio);
	getTime(time);
	//Log that the GPIO pins were freed
	PRINT_MSG(logFile, time, programName, "The GPIO pins have been freed\n\n");
	
	//Return to end the program
	return 0;
}









