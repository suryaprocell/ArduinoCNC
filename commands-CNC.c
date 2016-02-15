//Project: CNC v2
//Homepage: www.HomoFaciens.de
//Author Norbert Heinz
//Version: 1.1
//Creation date: 16.12.2015
//This program is free software you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation version 3 of the License.
//This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//For a copy of the GNU General Public License see http://www.gnu.org/licenses/
//
//compile with gcc commands-CNC.c -o commands-CNC -lm
//For details see:
//http://www.HomoFaciens.de/technics-machines-cnc-v2_en_navion.htm

#include <stdio.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <dirent.h>
#include <math.h>
#include <unistd.h>
#include <fcntl.h>



#define BAUDRATE B115200
//#define ARDUINOPORT "/dev/ttyUSB0"
#define ARDUINOPORT "/dev/ttyACM0"
#define FALSE 0
#define TRUE 1

#define BUFFER_MAX 30

#define PI 3.1415927

int KeyHit = 0;
int KeyCode[5];
int SingleKey=0;


int  MaxRows = 24;
int  MaxFileRows = 10;
int  FilesFound = 0;
int  MaxCols = 80;
int  MessageX = 1;
int  MessageY = 24;
double Scale = 1.0;
double OldScale = 1.0;
long MoveLength = 1;
long xMin = 1000000, xMax = -1000000;
long yMin = 1000000, yMax = -1000000;
char FileName[1000] = "";
long stepPause = 10000;
long currentPlotX = 0, currentPlotY = 0, currentPlotDown = 0;

long StepX = 0; 
long StepY = 0;
long StepZ = 0;
double StepsPermm = 1000.0 / 31.25;
double StepsPermmZ = 1000.0 / 31.25;
long zHubUp = 0, zHubDown = -100;

int fd = 0;
struct termios TermOpt;

int relayOn = 0;
int OldAdvanceRate = 0;
int AdvanceRate = 100;
int AdvanceRateArduino = 1;

int plotterSteps = 0;


char PicturePath[1000];




//+++++++++++++++++++++++ Start gotoxy ++++++++++++++++++++++++++
//Thanks to 'Stack Overflow', found on http://www.daniweb.com/software-development/c/code/216326
int gotoxy(int x, int y) {
  char essq[100]; // String variable to hold the escape sequence
  char xstr[100]; // Strings to hold the x and y coordinates
  char ystr[100]; // Escape sequences must be built with characters
   
  //Convert the screen coordinates to strings.
  sprintf(xstr, "%d", x);
  sprintf(ystr, "%d", y);
   
  //Build the escape sequence (vertical move).
  essq[0] = '\0';
  strcat(essq, "\033[");
  strcat(essq, ystr);
   
  //Described in man terminfo as vpa=\E[%p1%dd. Vertical position absolute.
  strcat(essq, "d");
   
  //Horizontal move. Horizontal position absolute
  strcat(essq, "\033[");
  strcat(essq, xstr);
  // Described in man terminfo as hpa=\E[%p1%dG
  strcat(essq, "G");
   
  //Execute the escape sequence. This will move the cursor to x, y
  printf("%s", essq);
  return 0;
}
//------------------------ End gotoxy ----------------------------------

//+++++++++++++++++++++++ Start clrscr ++++++++++++++++++++++++++
void clrscr(int StartRow, int EndRow) {
  int i, i2;
  
  if (EndRow < StartRow){
    i = EndRow;
    EndRow = StartRow;
    StartRow = i;
  }
  gotoxy(1, StartRow);
  for (i = 0; i <= EndRow - StartRow; i++){
    for(i2 = 0; i2 < MaxCols; i2++){
      printf(" ");
    }
    printf("\n");
  }
}
//----------------------- End clrscr ----------------------------

//+++++++++++++++++++++++ Start kbhit ++++++++++++++++++++++++++++++++++
//Thanks to Undertech Blog, http://www.undertec.de/blog/2009/05/kbhit_und_getch_fur_linux.html
int kbhit(void) {

   struct termios term, oterm;
   int fd = 0;
   int c = 0;
   
   tcgetattr(fd, &oterm);
   memcpy(&term, &oterm, sizeof(term));
   term.c_lflag = term.c_lflag & (!ICANON);
   term.c_cc[VMIN] = 0;
   term.c_cc[VTIME] = 1;
   tcsetattr(fd, TCSANOW, &term);
   c = getchar();
   tcsetattr(fd, TCSANOW, &oterm);
   if (c != -1)
   ungetc(c, stdin);

   return ((c != -1) ? 1 : 0);

}
//------------------------ End kbhit -----------------------------------

//+++++++++++++++++++++++ Start getch ++++++++++++++++++++++++++++++++++
//Thanks to Undertech Blog, http://www.undertec.de/blog/2009/05/kbhit_und_getch_fur_linux.html
int getch(){
   static int ch = -1, fd = 0;
   struct termios new, old;

   fd = fileno(stdin);
   tcgetattr(fd, &old);
   new = old;
   new.c_lflag &= ~(ICANON|ECHO);
   tcsetattr(fd, TCSANOW, &new);
   ch = getchar();
   tcsetattr(fd, TCSANOW, &old);

//   printf("ch=%d ", ch);

   return ch;
}
//------------------------ End getch -----------------------------------

//++++++++++++++++++++++ Start MessageText +++++++++++++++++++++++++++++
void MessageText(char *message, int x, int y, int alignment){
  int i;
  char TextLine[300];

  clrscr(y, y);
  gotoxy (x, y);
  
  TextLine[0] = '\0';
  if(alignment == 1){
    for(i=0; i < (MaxCols - strlen(message)) / 2 ; i++){
      strcat(TextLine, " ");
    }
  }
  strcat(TextLine, message);
  
  printf("%s\n", TextLine);
}
//-------------------------- End MessageText ---------------------------

//++++++++++++++++++++++ Start PrintRow ++++++++++++++++++++++++++++++++
void PrintRow(char character, int y){
  int i;
  gotoxy (1, y);
  for(i=0; i<MaxCols;i++){
    printf("%c", character);
  }
}
//-------------------------- End PrintRow ------------------------------

//+++++++++++++++++++++++++ ErrorText +++++++++++++++++++++++++++++
void ErrorText(char *message){
  clrscr(MessageY + 2, MessageY + 2);
  gotoxy (1, MessageY + 2);  
  printf("Last error: %s", message);
}
//----------------------------- ErrorText ---------------------------

//+++++++++++++++++++++++++ PrintMenue_01 ++++++++++++++++++++++++++++++
void PrintMenue_01(){
  char TextLine[300];
  
   clrscr(1, MessageY-2);
   MessageText("*** Main menu plotter ***", 1, 1, 1);
   sprintf(TextLine, "M            - toggle move length, current value = %ld step(s)", MoveLength);
   MessageText(TextLine, 10, 3, 0);
   MessageText("Cursor right - move plotter in positive X direction", 10, 4, 0);
   MessageText("Cursor left  - move plotter in negative X direction", 10, 5, 0);
   MessageText("Cursor up    - move plotter in positive Y direction", 10, 6, 0);
   MessageText("Cursor down  - move plotter in negative Y direction", 10, 7, 0);
   MessageText("Page up      - lift miller", 10, 8, 0);
   MessageText("Page down    - touch down miller", 10, 9, 0);
   sprintf(TextLine, "Z            - set Z hub of miller: %ld Steps HubU=%ld, HubD=%ld, Z=%ld", zHubUp-zHubDown, zHubUp, zHubDown, StepZ);
   MessageText(TextLine, 10, 10, 0);
   sprintf(TextLine, "- / +        - decrease or increase lowest point of miller for 0.5mm");
   MessageText(TextLine, 10, 11, 0);
   sprintf(TextLine, "A            - set advance rate if miller down (ms between steps): %d ms", AdvanceRate);
   MessageText(TextLine, 10, 12, 0);
   if(relayOn == 0){
     sprintf(TextLine, "R            - Relay ON/OFF, currently: OFF");
   }
   else{
     sprintf(TextLine, "R            - Relay ON/OFF, currently: ON");
   }
   MessageText(TextLine, 10, 13, 0);
   MessageText("G            - create gearwheel file (GearWheel.svg).", 10, 14, 0);
   sprintf(TextLine, "F            - choose file: \"%s\"", FileName);
   MessageText(TextLine, 10, 15, 0);
   sprintf(TextLine, "S            - scale: %0.4f. W = %0.2fcm, H = %0.2fcm", Scale, (xMax - xMin) * Scale / 1000.0, (yMax - yMin) * Scale / 1000.0);
   MessageText(TextLine, 10, 16, 0);
   MessageText("P            - plot file", 10, 17, 0);

   MessageText("Esc          - leave program", 10, 18, 0);
   
}
//------------------------- PrintMenue_01 ------------------------------

//+++++++++++++++++++++++++ PrintMenue_02 ++++++++++++++++++++++++++++++
char *PrintMenue_02(int StartRow, int selected){
  char TextLine[300];
  char OpenDirName[1000];
  static char FileName[101];
  DIR *pDIR;
  struct dirent *pDirEnt;
  int i = 0;  
  int Discard = 0;
  
  clrscr(1, MessageY-2);
  MessageText("*** Choose plotter file ***", 1, 1, 1);
   
  strcpy(OpenDirName, PicturePath);
  

  pDIR = opendir(OpenDirName);
  if ( pDIR == NULL ) {
    sprintf(TextLine, "Could not open directory '%s'!", OpenDirName);
    MessageText(TextLine, 1, 4, 1);
    getch();
    return( "" );
  }

  FilesFound = 0;
  pDirEnt = readdir( pDIR );
  while ( pDirEnt != NULL && i < MaxFileRows) {
    //printf("%s\n", pDirEnt->d_name);
    if(strlen(pDirEnt->d_name) > 4){
      if(memcmp(pDirEnt->d_name + strlen(pDirEnt->d_name)-4, ".svg",4) == 0){
        FilesFound++;
        if(Discard >= StartRow){
          if(i + StartRow == selected){
            sprintf(TextLine, ">%s<", pDirEnt->d_name);
            strcpy(FileName, pDirEnt->d_name);
          }
          else{
            sprintf(TextLine, " %s ", pDirEnt->d_name); 
          }
          MessageText(TextLine, 1, 3 + i, 0);
          i++;
        }
        Discard++;
      }
    }
    pDirEnt = readdir( pDIR );
  }  

  gotoxy(MessageX, MessageY + 1);
  printf("Choose file using up/down keys and confirm with 'Enter' or press 'Esc' to cancel.");
  

  return (FileName);
}
//------------------------- PrintMenue_02 ------------------------------


//+++++++++++++++++++++++++ PrintMenue_03 ++++++++++++++++++++++++++++++
void PrintMenue_03(char *FullFileName, long CurrentLine, long NumberOfLines, long CurrentX, long CurrentY, long StartTime){
  char TextLine[300];
  long CurrentTime, ProcessHours = 0, ProcessMinutes = 0, ProcessSeconds = 0;
  
   CurrentTime = time(0);
   
   CurrentTime -= StartTime;
   
   while (CurrentTime > 3600){
     ProcessHours++;
     CurrentTime -= 3600;
   }
   while (CurrentTime > 60){
     ProcessMinutes++;
     CurrentTime -= 60;
   }
   ProcessSeconds = CurrentTime;
   
   clrscr(1, MessageY - 2);
   MessageText("*** Plotting file ***", 1, 1, 1);
   
   sprintf(TextLine, "File name: %s", FullFileName);
   MessageText(TextLine, 10, 3, 0);
   sprintf(TextLine, "Total number of lines: %ld", NumberOfLines);
   MessageText(TextLine, 10, 4, 0);
   sprintf(TextLine, "Current Line(%ld): X = %ld, Y = %ld     ", CurrentLine, CurrentX, CurrentY);
   MessageText(TextLine, 10, 5, 0);
   sprintf(TextLine, "Process time: %02ld:%02ld:%02ld", ProcessHours, ProcessMinutes, ProcessSeconds);
   MessageText(TextLine, 10, 6, 0);
     

}
//------------------------- PrintMenue_03 ------------------------------


//+++++++++++++++++++++++ Start readport ++++++++++++++++++++++++++
char  readport(void){
  int n;
	char buff;
  n = read(fd, &buff, 1);
  if(n > 0){
    return buff;
  }
  return 0;
}
//------------------------ End readport ----------------------------------

//+++++++++++++++++++++++ Start sendport ++++++++++++++++++++++++++
void sendport(unsigned char ValueToSend){
  int n;

  n = write(fd, &ValueToSend, 1);
  
  if (n < 0){
    ErrorText("write() of value failed!\r");
  }
  else{
    while(readport() != 'r');
  }
  
}
//------------------------ End sendport ----------------------------------

//+++++++++++++++++++++++ Start openport ++++++++++++++++++++++++++
void openport(void){
    
    fd = open(ARDUINOPORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)  {
      ErrorText("init_serialport: Unable to open port ");
    }
    
    if (tcgetattr(fd, &TermOpt) < 0) {
      ErrorText("init_serialport: Couldn't get term attributes");
    }
    speed_t brate = BAUDRATE; // let you override switch below if needed

    cfsetispeed(&TermOpt, brate);
    cfsetospeed(&TermOpt, brate);

    // 8N1
    TermOpt.c_cflag &= ~PARENB;
    TermOpt.c_cflag &= ~CSTOPB;
    TermOpt.c_cflag &= ~CSIZE;
    TermOpt.c_cflag |= CS8;
    // no flow control
    TermOpt.c_cflag &= ~CRTSCTS;

    TermOpt.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    TermOpt.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    TermOpt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    TermOpt.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    TermOpt.c_cc[VMIN]  = 0;
    TermOpt.c_cc[VTIME] = 20;
    
    if( tcsetattr(fd, TCSANOW, &TermOpt) < 0) {
      ErrorText("init_serialport: Couldn't set term attributes");
    }

} 
//------------------------ End openport ----------------------------------


//++++++++++++++++++++++++++++++ moveZ ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void moveZ(long targetZ, int stopPlot){

  if(stopPlot == 0){
    while(StepZ != targetZ){
      if(StepZ > targetZ){
        sendport(16);
        StepZ--;
      }
      else{
        sendport(32);
        StepZ++;
      }
    }
  }

}


//++++++++++++++++++++++++++++++ MakeStepX ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void MakeStepX(int direction){
  StepX += direction;
  plotterSteps++;
  
  if(direction > 0){
    sendport(1);
  }
  else{
    sendport(2);
  }
}

//++++++++++++++++++++++++++++++ MakeStepY ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void MakeStepY(int direction){
  StepY += direction;
  plotterSteps++;
  
  if(direction > 0){
    sendport(4);
  }
  else{
    sendport(8);
  }  
}



//++++++++++++++++++++++++++++++++++++++ CalculatePlotter ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
int CalculatePlotter(long moveX, long moveY, long stepPause, int stopPlot){
  char TextLine[1000] = "";
  long  tempX = 0, tempY = 0, stepsDone = 0;
  int i = 0, i2=0;
  
  if(moveX == 0 && moveY == 0){
    return 0;
  }
  
  sprintf(TextLine, "Moving X: %ld, Moving Y: %ld", moveX, moveY);
  MessageText(TextLine, MessageX, MessageY, 0);
//  getch();

  if(stopPlot == 0){
    if(moveX == 0){
      if(moveY > 0){
        for(i = 0; i < moveY; i++){
           MakeStepY(-1);
        }
      }
      if(moveY < 0){
        for(i = 0; i < -moveY; i++){
           MakeStepY(1);
        }
      }
    }
    if(moveY == 0){
      if(moveX > 0){
        for(i = 0; i < moveX; i++){
           MakeStepX(1);
        }
      }
      if(moveX < 0){
        for(i = 0; i < -moveX; i++){
           MakeStepX(-1);
        }
      }
    }
    if(moveY != 0 && moveX != 0){
      if(abs(moveX) > abs(moveY)){
        for(i = 0; i < abs(moveX); i++){
          if(moveX > 0){
            MakeStepX(1);
          }
          if(moveX < 0){
            MakeStepX(-1);
          }
          tempY = 0.5 + (double)((i + 1) * abs(moveY)) / (double)abs(moveX) - stepsDone;
          if(moveY < 0){
            tempY = -tempY;
          }
          for(i2=0; i2 < abs(tempY); i2++){
            if(tempY > 0){
              MakeStepY(-1);
              stepsDone++;
            }
            if(tempY < 0){
              MakeStepY(1);
              stepsDone++;
            }
          }
        }
        if(moveY > 0){
          moveY -= stepsDone;
        }
        else{
          moveY += stepsDone;
        }
        //Eventually remaining Y coordinates;
        while(moveY > 0){
          MakeStepY(-1);
          moveY--;
        }
        while(moveY < 0){
          MakeStepY(1);
          moveY++;
        }
      }//if(abs(moveX) > abs(moveY))
      else{
        for(i = 0; i < abs(moveY); i++){
          if(moveY > 0){
            MakeStepY(-1);
          }
          if(moveY < 0){
            MakeStepY(1);
          }
          tempX = 0.5 + (double)(i * abs(moveX)) / (double)abs(moveY) - stepsDone;
          if(moveX < 0){
            tempX = -tempX;
          }
          for(i2=0; i2 < abs(tempX); i2++){
            if(tempX > 0){
              MakeStepX(1);
              stepsDone++;
            }
            if(tempX < 0){
              MakeStepX(-1);
              stepsDone++;
            }
          }
        }
        //Eventually remaining X coordinates;
        if(moveX > 0){
          moveX -= stepsDone;
        }
        else{
          moveX += stepsDone;
        }
        while(moveX > 0){
          MakeStepX(1);
          moveX--;
        }
        while(moveX < 0){
          MakeStepX(-1);
          moveX++;
        }
      }
    }
  }

  if(kbhit()){
    if(getch() == 27){
      return 1;
    }
  }
  return 0;
}
//-------------------------------------- CalculatePlotter --------------------------------------------------------

//++++++++++++++++++++++++++++++++++++++ KeyControl ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void KeyControl(int action){
int i;

  i = 0;
  SingleKey = 1;
  KeyCode[0] = 0;
  KeyCode[1] = 0;
  KeyCode[2] = 0;
  KeyCode[3] = 0;
  KeyCode[4] = 0;
  KeyHit = 0;
  while (kbhit()){
    KeyHit = getch();
    KeyCode[i] = KeyHit;
    i++;
    if(i == 5){
      i = 0;
    }
    if(i > 1){
      SingleKey = 0;
    }
  }
  if(SingleKey == 0){
    KeyHit = 0;
  }
  
  if(action == 1 || action == 2){
    //Move X-axis
    if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 68 && KeyCode[3] == 0 && KeyCode[4] == 0){
      CalculatePlotter(-MoveLength, 0, stepPause, 0);
    }

    if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 67 && KeyCode[3] == 0 && KeyCode[4] == 0){
      CalculatePlotter(MoveLength, 0, stepPause, 0);
    }

    //Move Y-axis
    if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 65 && KeyCode[3] == 0 && KeyCode[4] == 0){
      CalculatePlotter(0, -MoveLength, stepPause, 0);
    }

    if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 66 && KeyCode[3] == 0 && KeyCode[4] == 0){
      CalculatePlotter(0, MoveLength, stepPause, 0);
    }

    if(action != 2){
		//Pen UP/DOWN
		if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 53 && KeyCode[3] == 126 && KeyCode[4] == 0){
		  moveZ(zHubUp, 0);
		  currentPlotDown = 0;
		}

		if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 54 && KeyCode[3] == 126 && KeyCode[4] == 0){
		  moveZ(zHubDown, 0);
		  currentPlotDown = 1;
		}
	}

    if(KeyHit == 'm'){
      MoveLength *= 10;
      if(MoveLength == 10000){
        MoveLength = 1;
      }
      PrintMenue_01();
    }

    if(KeyHit == 'r'){
      if(relayOn == 1){
        sendport(254);//Turn relay off
        relayOn = 0;
      }
      else{
        sendport(255);//Turn relay on
        relayOn = 1;
      }
      PrintMenue_01();
    }

  }
  
  
}
//-------------------------------------- KeyControl --------------------------------------------------------


//######################################################################
//################## Main ##############################################
//######################################################################

int main(int argc, char **argv){

  int MenueLevel = 0;
  char FullFileName[1000] = "";
  char FileNameOld[1000] = "";
  struct winsize terminal;
  int i;
  int FileSelected = 0;
  int FileStartRow = 0;
  char *pEnd;
  FILE *PlotFile;
  char TextLine[30000];
  long coordinateCount = 0;
  long maxCoordinateCount = 0;
  char a;
  int ReadState = 0;
  long xNow = 0, yNow = 0;
  long xNow1 = 0, yNow1 = 0;
  long xNow2 = 0, yNow2 = 0;
  long coordinatePlot = 0;
  int stopPlot = 0;
  long PlotStartTime = 0;
  long zHubUpOld = 0, zHubDownOld = 0;

  openport();

  printf("\n\nWaiting for 'X' from Arduino (Arduino pluged in?)...\n");

  //Wait for 'X' from Arduino
  while(readport() != 'X');


  strcpy(FileName, "noFiLE");

  getcwd(PicturePath, 1000);
  strcat(PicturePath, "/pictures");
  printf("PicturePath=>%s<", PicturePath);

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &terminal)<0){
    printf("Can't get size of terminal window");
  }
  else{
    MaxRows = terminal.ws_row;
    MaxCols = terminal.ws_col;
    MessageY = MaxRows-3;
  }

  clrscr(1, MaxRows);
  PrintRow('-', MessageY - 1);
  PrintMenue_01();

  MaxFileRows = MaxRows - 10;

  while (1){
    MessageText("Waiting for key press.", MessageX, MessageY, 0);

    if(MenueLevel == 0){
      KeyControl(1);

      if(KeyHit == 'f'){
        strcpy(FileNameOld, FileName);
        strcpy(FileName, PrintMenue_02(FileStartRow, FileSelected));
        MenueLevel = 1;
      }

      if(KeyHit == 's'){
        OldScale = Scale;
        MessageText("Type new scale value: ", 1, MessageY, 0);
        gotoxy(23, MessageY);
        scanf("%lf", &Scale);
        if(Scale == 0){
          Scale = OldScale;
        }
        else{
          PrintMenue_01();
        }
      }

      if(KeyHit == 'a'){
        OldAdvanceRate = AdvanceRate;
        MessageText("Type new advance rate: ", 1, MessageY, 0);
        gotoxy(23, MessageY);
        scanf("%d", &AdvanceRate);
        if(AdvanceRate == 0){
          AdvanceRate = OldAdvanceRate;
        }
        else{
          PrintMenue_01();
        }
      }


      if(KeyHit == 'g'){
        FILE *FileOut;
        double gearModule = 0.0;
        double toothDiameter = 0.0;
        double gearTeeth = 0.0;
        double gearHole = 0.0;
        double gearMiller = 0.0;
        double r = 0.0;
        double x, y;
        double alpha = 0;
        double beta = 0.0;
        char Zeile[100000];
        char fileName[10000];
        double faktor = 100.0;
        double lastX, lastY;
        double firstX, firstY;
        double closePathX, closePathY;
        
        MessageText("Type tooth diameter in mm: ", 1, MessageY, 0);
        gotoxy(28, MessageY);
        scanf("%lf", &toothDiameter);
        toothDiameter /= 2.0;
        if(toothDiameter > 0.0){
          MessageText("Type number of teeth: ", 1, MessageY, 0);
          gotoxy(23, MessageY);
          scanf("%lf", &gearTeeth);
          gearTeeth = (int)gearTeeth;
          if(gearTeeth > 0.0){
            MessageText("Type diameter of center hole in mm: ", 1, MessageY, 0);
            gotoxy(37, MessageY);
            scanf("%lf", &gearHole);
            gearHole /= 2.0;
            if(gearHole > 0.0){
              MessageText("Type diameter of miller in mm: ", 1, MessageY, 0);
              gotoxy(32, MessageY);
              scanf("%lf", &gearMiller);
              gearMiller /= 2.0;
              if(gearMiller > 0.0){
             
                strcpy(fileName, PicturePath);
                strcat(fileName, "/GearWheel.svg");

                double r_a, r_b;
                r_a = sin(PI / (gearTeeth * 4.0)) * toothDiameter / 2.0;
                r_b = (cos(PI / (gearTeeth * 4.0)) * toothDiameter / 2.0) / (tan(PI / (gearTeeth * 4.0)));
                r = r_a + r_b;                
                
                if((FileOut=fopen(fileName,"wb"))==NULL){
                  printf("Can't open file '%s'!\n", fileName);
                  return(1);
                }
                else{
                  strcpy(Zeile, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg version=\"1.1\" viewBox=\"0 0 21000 29700\" preserveAspectRatio=\"xMidYMid\" fill-rule=\"evenodd\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\"><g visibility=\"visible\" id=\"Default\"><desc>Master slide</desc><g style=\"stroke:none;fill:none\"><rect x=\"-14\" y=\"-14\" width=\"21030\" height=\"29730\"/></g></g><g visibility=\"visible\" id=\"page1\"><desc>Slide</desc><g><desc>Drawing</desc><g><g style=\"stroke:rgb(0,0,0);fill:none\">");
                  fwrite(Zeile, 1, strlen(Zeile), FileOut);
                  
                  //Create path of hole
                  sprintf(Zeile, "<path style=\"fill:none\" d=\"M ");
                  fwrite(Zeile, 1, strlen(Zeile), FileOut);
                  for(alpha = 0.0; alpha < 2.0 * PI; alpha+=0.05){
                    x = sin(alpha) * (gearHole - gearMiller);
                    y = cos(alpha) * (gearHole - gearMiller) - r;
                    sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                    fwrite(Zeile, 1, strlen(Zeile), FileOut);      
                  }
                  x = sin(0.0) * (gearHole - gearMiller);
                  y = cos(0.0) * (gearHole - gearMiller) - r;
                  sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                  fwrite(Zeile, 1, strlen(Zeile), FileOut);      
                  
                  
                  //Create path of gears
                  sprintf(Zeile, "\"/></g><g/></g></g><g><desc>Drawing</desc><g><g style=\"stroke:rgb(0,0,0);fill:none\"><path style=\"fill:none\" d=\"M ");
                  fwrite(Zeile, 1, strlen(Zeile), FileOut);
                  firstX = 0.0;
                  firstY = 0.0;
                  lastX = 0.0;
                  lastY = 0.0;
                  closePathX = -99999.9;
                  closePathY = -99999.9;
                  for(i = 0; i < gearTeeth; i++){
                    beta =  0.0 - PI / gearTeeth / 8.0 + i * 2.0 * PI / gearTeeth;
                    r = (toothDiameter + gearMiller);
                    for(alpha = 0.0 - PI / gearTeeth / 2.0 + beta; alpha < PI + PI / gearTeeth / 2.0 + beta; alpha+=0.1){
                      if(firstX == -99999.0 && firstY == -99999.0){
                        firstX = r * cos(alpha);
                        firstY = r * sin(alpha);
                      }
                      x = r * cos(alpha) + lastX - firstX;
                      y = r * sin(alpha) + lastY - firstY;
                      sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                      fwrite(Zeile, 1, strlen(Zeile), FileOut);
                      if(closePathX == -99999.9 && closePathY == -99999.9){
                        closePathX = x;
                        closePathY = y;
                      }
                    }
                    x = r * cos(PI + PI / gearTeeth + beta) + lastX - firstX;
                    y = r * sin(PI + PI / gearTeeth + beta) + lastY - firstY;
                    lastX = x;
                    lastY = y;
                    sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                    fwrite(Zeile, 1, strlen(Zeile), FileOut);
                    firstX = 0.0;
                    firstY = 0.0;
                    r = (toothDiameter - gearMiller);
                    for(alpha = 2.0 * PI - PI / gearTeeth / 2.0 + PI / gearTeeth + beta; alpha > PI + PI / gearTeeth / 2.0 + PI / gearTeeth + beta; alpha-=0.1){
                      if(firstX == 0.0 && firstY == 0.0){
                        firstX = r * cos(alpha);
                        firstY = r * sin(alpha);
                      }
                      x = r * cos(alpha) + lastX - firstX;
                      y = r * sin(alpha) + lastY - firstY;
                      sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                      fwrite(Zeile, 1, strlen(Zeile), FileOut);
                    }
                    x = r * cos(PI + PI / gearTeeth / 2.0 + PI / gearTeeth + beta) + lastX - firstX;
                    y = r * sin(PI + PI / gearTeeth / 2.0 + PI / gearTeeth + beta) + lastY - firstY;
                    lastX = x;
                    lastY = y;
                    sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                    fwrite(Zeile, 1, strlen(Zeile), FileOut);
                    firstX = -99999.0;
                    firstY = -99999.0;
                  }
                  
                  x=closePathX;
                  y=closePathY;
                  sprintf(Zeile, " %ld,%ld ", (long)(x * faktor), (long)(y * faktor));
                  fwrite(Zeile, 1, strlen(Zeile), FileOut);
                  
                  sprintf(Zeile, "\"/></g><g/></g></g></g></svg>");
                  fwrite(Zeile, 1, strlen(Zeile), FileOut);
                }
                fclose(FileOut);

                while(kbhit()){
                  getch();
                }
                MessageText("File 'GearWheel.svg successfully created. Press any key to continue...", 1, MessageY, 0);
                getch();
                
              }//if(gearMiller > 0.0){
            }//if(gearHole > 0.0){
          }//if(gearTeeth > 0.0){
        }//if(gearModule > 0.0){
      }//if(KeyHit == 'g'){


      if(KeyHit == '+'){
        zHubDown+=StepsPermmZ / 2.0;
        PrintMenue_01();
      }

      if(KeyHit == '-'){
        zHubDown-=StepsPermmZ / 2.0;
        PrintMenue_01();
      }

      if(KeyHit == 'z'){
        zHubUpOld = zHubUp;
        zHubDownOld = zHubDown;
        OldScale = Scale;
        MessageText("Move to deepest point and press 'ENTER'. ", 1, MessageY, 0);
        zHubDown = StepZ;
        while(KeyHit != 10 && KeyHit != 27){
          KeyControl(2);
                  
          if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 53 && KeyCode[3] == 126 && KeyCode[4] == 0){
            zHubDown += MoveLength;
            moveZ(zHubDown, 0);
          }
          if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 54 && KeyCode[3] == 126 && KeyCode[4] == 0){
            zHubDown -= MoveLength;
            moveZ(zHubDown, 0);
          }
        }
        if(KeyHit == 10){
          zHubUp = StepZ;
          KeyHit = 0;
          currentPlotDown = 1;
          MessageText("Move to highest point and press 'ENTER'. ", 1, MessageY, 0);
          while(KeyHit != 10 && KeyHit != 27){
            KeyControl(2);
          
            if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 53 && KeyCode[3] == 126 && KeyCode[4] == 0){
              zHubUp += MoveLength;
              moveZ(zHubUp, 0);
            }
            if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 54 && KeyCode[3] == 126 && KeyCode[4] == 0){
              zHubUp -= MoveLength;
              moveZ(zHubUp, 0);
            }
          }
        }//if(KeyHit == 10){
        if(KeyHit == 10){
          if(zHubUp < zHubDown){
            zHubDownOld = zHubDown;
            zHubDown = zHubUp;
            zHubUp = zHubDownOld;
          }
        }
        else{
          zHubUp = zHubUpOld;
          zHubDown = zHubDownOld;
        }
        sendport(254);//Turn relay off
        relayOn = 0;
        moveZ(zHubUp, 0);
        currentPlotDown = 0;
        PrintMenue_01();
      }//if(KeyHit == 'z'){


      if(KeyHit == 'p'){//Plot file
        MessageText("3 seconds until plotting starts !!!!!!!!!!!!!!!!!", 1, 20, 0);
        sleep(3);
        if(kbhit()){
          getch();
          KeyHit = 27;
        }
        if(strcmp(FileName, "noFiLE") != 0){
          if((PlotFile=fopen(FullFileName,"rb"))==NULL){
            sprintf(TextLine, "Can't open file '%s'!\n", FullFileName);
            strcpy(FileName, "NoFiLE");
            ErrorText(TextLine);
          }
        }
        if(strcmp(FileName, "noFiLE") != 0 && KeyHit != 27){
          xNow1 = -1;
          xNow2 = -1;
          yNow1 = -1;
          yNow2 = -1;
          currentPlotX = 0;
          currentPlotY = 0;        
          PlotStartTime = time(0);
          coordinatePlot = 0;
          coordinateCount = 0;
          stopPlot = 0;
          if(currentPlotDown == 1){
            sendport(254);//Turn relay off
            relayOn = 0;
            moveZ(zHubUp, 0);
            currentPlotDown = 0;
          }
          PrintMenue_03(FullFileName, coordinateCount, maxCoordinateCount, StepX, StepY, PlotStartTime);
          
          while(!(feof(PlotFile)) && stopPlot == 0){
            fread(&a, 1, 1, PlotFile);
            i=0;
            TextLine[0] = '\0';
            while(!(feof(PlotFile)) && a !=' ' && a != '<' && a != '>' && a != '\"' && a != '=' && a != ',' && a != ':' && a != 10){
              TextLine[i] = a;
              TextLine[i+1] = '\0';
              i++;
              fread(&a, 1, 1, PlotFile);
            }
            if(a == '<'){//Init
              if(xNow2 > -1 && yNow2 > -1 && (xNow2 != xNow1 || yNow2 != yNow1)){
                stopPlot = CalculatePlotter(xNow2 - currentPlotX, yNow2 - currentPlotY, stepPause, stopPlot);
                if(currentPlotDown == 0){
                  sendport(255);//Turn relay on
                  relayOn = 1;
                  moveZ(zHubDown, stopPlot);
                  currentPlotDown = 1;                  
                  while (AdvanceRateArduino < AdvanceRate){
                    sendport(128);//Raise step pause
                    AdvanceRateArduino+=10;
                  }
                }
                currentPlotX = xNow2;
                currentPlotY = yNow2;

                stopPlot = CalculatePlotter(xNow1 - currentPlotX, yNow1 - currentPlotY, stepPause, stopPlot);
                currentPlotX = xNow1;
                currentPlotY = yNow1;

                stopPlot = CalculatePlotter(xNow - currentPlotX, yNow - currentPlotY, stepPause, stopPlot);
                currentPlotX = xNow;
                currentPlotY = yNow;

                sendport(255);//Turn relay on
                relayOn = 1;
                plotterSteps = 9999999;
              }
              ReadState = 0;
              xNow1 = -1;
              xNow2 = -1;
              yNow1 = -1;
              yNow2 = -1;
            }
            if(strcmp(TextLine, "path") == 0){
              if(currentPlotDown == 1){
                sendport(254);//Turn relay off
                relayOn = 0;
                while(AdvanceRateArduino > 1){
                  sendport(64);//Reduce step pause
                  AdvanceRateArduino-=10;
                }
                moveZ(zHubUp, stopPlot);
                currentPlotDown = 0;
              }
              ReadState = 1;//path found
            }
            if(ReadState == 1 && strcmp(TextLine, "fill") == 0){
              ReadState = 2;//fill found
            }
            if(ReadState == 2 && strcmp(TextLine, "none") == 0){
              ReadState = 3;//none found
            }
            if(ReadState == 2 && strcmp(TextLine, "stroke") == 0){
              ReadState = 0;//stroke found, fill isn't "none"
            }
            if(ReadState == 3 && strcmp(TextLine, "d") == 0 && a == '='){
              ReadState = 4;//d= found
            }
            if(ReadState == 4 && strcmp(TextLine, "M") == 0 && a == ' '){
              ReadState = 5;//M found
            }

            if(ReadState == 6){//Y value
              double doubleTemp;
              doubleTemp = strtol(TextLine, &pEnd, 10) - yMin;
              doubleTemp *= StepsPermm;
              doubleTemp *= Scale;
              doubleTemp /= 100.0;
              yNow = doubleTemp;
              //yNow = yMax - yNow;//Mirror y-axis
              //yNow = (long)((double)((strtol(TextLine, &pEnd, 10) - yMin)) * StepsPermm * Scale / 100.0);
              ReadState = 7;
              coordinateCount++;
              PrintMenue_03(FullFileName, coordinateCount, maxCoordinateCount, StepX, StepY, PlotStartTime);
            }
            if(ReadState == 5 && a == ','){//X value
              double doubleTemp;
              doubleTemp = strtol(TextLine, &pEnd, 10) - xMin;
              //doubleTemp = xMax - strtol(TextLine, &pEnd, 10);//Mirror x-axis
              doubleTemp *= StepsPermm;
              doubleTemp *= Scale;
              doubleTemp /= 100.0;
              xNow = doubleTemp;
              //xNow = (long)((double)(((xMax - strtol(TextLine, &pEnd, 10)))) * StepsPermm * Scale / 100.0);
              ReadState = 6;
            }
            if(ReadState == 7){
              if(xNow2 > -1 && yNow2 > -1 && (xNow2 != xNow1 || yNow2 != yNow1)){
                stopPlot = CalculatePlotter(xNow2 - currentPlotX, yNow2 - currentPlotY, stepPause, stopPlot);
                if(currentPlotDown == 0){
                  sendport(255);//Turn relay on
                  relayOn = 1;
                  moveZ(zHubDown, stopPlot);
                  currentPlotDown = 1;                  
                  while (AdvanceRateArduino < AdvanceRate){
                    sendport(128);//Raise step pause
                    AdvanceRateArduino+=10;
                  }
                }
                currentPlotX = xNow2;
                currentPlotY = yNow2;
              }
              xNow2 = xNow1;
              yNow2 = yNow1;
              xNow1 = xNow;
              yNow1 = yNow;
              ReadState = 5;
            }
          }//while(!(feof(PlotFile)) && stopPlot == 0){
          fclose(PlotFile);
          if(currentPlotDown == 1){
            sendport(254);//Turn relay off
            relayOn = 0;
            while(AdvanceRateArduino > 10){
              sendport(64);//Reduce step pause
              AdvanceRateArduino-=10;
            }
            moveZ(zHubUp, 0);
            currentPlotDown = 0;
          }
          PrintMenue_03(FullFileName, coordinateCount, maxCoordinateCount, StepX, StepY, PlotStartTime);
          CalculatePlotter( -currentPlotX, -currentPlotY, stepPause, 0);
          currentPlotX = 0;
          currentPlotY = 0;
          while(kbhit()){
            getch();
          }
          MessageText("Finished! Press any key to return to main menu.", MessageX, MessageY, 0);
          getch();
          PrintMenue_01();
            
        }//if(strcmp(FileName, "noFiLE") != 0){
      }//if(KeyHit == 'p'){


    }//if(MenueLevel == 0){

    if(MenueLevel == 1){//Select file
      KeyControl(0);
      if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 66 && KeyCode[3] == 0 && KeyCode[4] == 0){
        if(FileSelected < FilesFound - 1){
          FileSelected++;
          if(FileSelected > MaxFileRows - 2){
            FileStartRow = FileSelected - MaxFileRows + 2;
          }
          strcpy(FileName, PrintMenue_02(FileStartRow, FileSelected));
        }
      }

      if(KeyCode[0] == 27 && KeyCode[1] == 91 && KeyCode[2] == 65 && KeyCode[3] == 0 && KeyCode[4] == 0){
        if(FileSelected > 0){
          if(FileSelected == FileStartRow + 1){
            if(FileStartRow > 0){
              FileStartRow--;
            }
          }
          FileSelected--;
          strcpy(FileName, PrintMenue_02(FileStartRow, FileSelected));
        }
      }

      if(KeyHit == 10){//Read file and store values
        MenueLevel = 0;
        clrscr(MessageY + 1, MessageY + 1);
        strcpy(FullFileName, PicturePath);
        strcat(FullFileName, "/");
        strcat(FullFileName, FileName);
        if((PlotFile=fopen(FullFileName,"rb"))==NULL){
          sprintf(TextLine, "Can't open file '%s'!\n", FullFileName);
          ErrorText(TextLine);
          strcpy(FileName, "NoFiLE");
        }
        else{
          xMin=1000000;
          xMax=-1000000;
          yMin=1000000;
          yMax=-1000000;
          maxCoordinateCount = 0;
          
            
          while(!(feof(PlotFile)) && stopPlot == 0){
            
            fread(&a, 1, 1, PlotFile);
            i=0;
            TextLine[0] = '\0';
            while(!(feof(PlotFile)) && a !=' ' && a != '<' && a != '>' && a != '\"' && a != '=' && a != ',' && a != ':' && a != 10){
              TextLine[i] = a;
              TextLine[i+1] = '\0';
              i++;
              fread(&a, 1, 1, PlotFile);
            }
            if(a == '<'){//Init
              ReadState = 0;
            }
            if(strcmp(TextLine, "path") == 0){
              ReadState = 1;//path found
            }
            if(ReadState == 1 && strcmp(TextLine, "fill") == 0){
              ReadState = 2;//fill found
            }
            if(ReadState == 2 && strcmp(TextLine, "none") == 0){
              ReadState = 3;//none found
            }
            if(ReadState == 2 && strcmp(TextLine, "stroke") == 0){
              ReadState = 0;//stroke found, fill isn't "none"
            }
            if(ReadState == 3 && strcmp(TextLine, "d") == 0 && a == '='){
              ReadState = 4;//d= found
            }
            if(ReadState == 4 && strcmp(TextLine, "M") == 0 && a == ' '){
              ReadState = 5;//M found
            }

            if(ReadState == 5 && strcmp(TextLine, "C") == 0 && a == ' '){
              ReadState = 5;//C found
            }

            if(ReadState == 6){//Y value
              yNow = strtol(TextLine, &pEnd, 10);
              //printf("String='%s' y=%ld\n", TextLine, yNow);
              if(yNow > yMax){
                yMax = yNow;
              }
              if(yNow < yMin){
                yMin = yNow;
              }
              ReadState = 7;
              maxCoordinateCount++;
            }
            if(ReadState == 5 && a == ','){//X value
              xNow = strtol(TextLine, &pEnd, 10);
              if(xNow > xMax){
                xMax = xNow;
              }
              if(xNow < xMin){
                xMin = xNow;
              }
              ReadState = 6;
            }
            if(ReadState == 7){              
              //printf("Found koordinates %ld, %ld\n", xNow, yNow);
              ReadState = 5;
            }
            gotoxy(1, MessageY);printf("ReadState=% 3d, xNow=% 10ld, xMin=% 10ld, xMax=% 10ld, yMin=% 10ld, yMax=% 10ld   ", ReadState, xNow, xMin, xMax, yMin, yMax);

          }//while(!(feof(PlotFile)) && stopPlot == 0){
          fclose(PlotFile);
          Scale = 1.0;
          //getch();
        }
        PrintMenue_01();
      }//if(KeyHit == 10){
    
    }//if(MenueLevel == 1){
    
        
    if(KeyHit == 27){
      if(MenueLevel == 0){
        clrscr(MessageY + 1, MessageY + 1);
        MessageText("Exit program (y/n)?", MessageX, MessageY + 1, 0);
        while(KeyHit != 'y' && KeyHit != 'n'){
          KeyHit = getch();
          if(KeyHit == 'y'){
            exit(0);
          }
        }
      }
      if(MenueLevel == 1){
        MenueLevel = 0;
        strcpy(FileName, FileNameOld);
        PrintMenue_01();
      }
      clrscr(MessageY + 1, MessageY + 1);
    }
  }

  return 0;
}
