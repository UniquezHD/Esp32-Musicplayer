#include "main.h"

// C:/Users/xxxx/.platformio/penv/Scripts/platformio.exe device monitor --environment upesy_wroom

// FIX sidste sang bliver sprunget over ved autoplay next
// FIX Russian keyboard characters de kan ikke være i char fordi de er for store
// FIX Russian bogstaver scroll

// TODO check for name length til at fixe hoppet n[r den ikke skal scroll
// TODO lav scroll til en function Scroll(SongName, how many characters to trigger (24))
// TODO n[r man v;lger en folder s[ s;tter den selectedFolder value til en anden variable
// s[ hvis shuffle er p[ skifter den ikke folder at shuffle i f'r den er valgt


//https://www.upesy.com/blogs/tutorials/esp32-pinout-reference-gpio-pins-ultimate-guide

U8G2_ST7565_ERC12864_1_4W_SW_SPI u8g2(U8G2_R0, /* scl=*/ 18, /* si=*/ 23, /* cs=*/ 5, /* rs=*/ 3, /* rse=*/ 4);

SFE_MAX1704X lipo(MAX1704X_MAX17043);// sda 22 scl 21

// mp3 rx2 og tx2

static int8_t Send_buf[8] = {0};
static uint8_t ansbuf[10] = {0}; 

int songIndex = 0;
int lastPlayedSongIndex;

int buttonUpClicked = 0; 
int buttonSelectClicked = 0; 
int buttonDownClicked = 0;

int selectedMonitorOption = 0;

int selectedIndex = 0; 
int previousSelectedIndex; 
int nextSelectedIndex; 
int next2SelectedIndex; 

int prevVolume = 0;
int volumeTick = 5;
int volumeMode = 1;

int selectedKeyboard = 0;
int selectedKey = 0;

const int RUS_KEYBOARD_LENGHT = 37;
const int ENG_KEYBOARD_LENGHT = 30;

char keyboardKeysEnglish[ENG_KEYBOARD_LENGHT] [2] = { 
  {"a"}, {"b"}, {"c"}, {"d"}, {"e"}, {"f"}, {"g"}, {"h"}, {"i"}, {"j"}, {"k"}, {"l"}, {"m"}, {"n"}, {"o"}, {"p"}, {"q"}, {"r"}, {"s"}, {"t"}, {"u"}, {"v"}, {"w"}, {"x"}, {"y"}, {"z"}, {"^"}, {"<"}, {">"}, {"X"}
};

char keyboardKeysRussian[RUS_KEYBOARD_LENGHT] [3] = { 
  {"а"}, {"б"}, {"в"}, {"г"}, {"д"}, {"е"}, {"ё"}, {"ж"}, {"з"}, {"и"}, {"й"}, {"к"}, {"л"}, {"м"}, {"н"}, {"о"}, {"п"}, {"р"}, {"с"}, {"т"}, {"у"}, {"ф"}, {"х"}, {"ц"}, {"ч"}, {"ш"}, {"щ"}, {"ъ"}, {"ы"}, {"ь"}, {"э"}, {"ю"}, {"я"}, {"^"}, {"<"}, {">"}, {"x"}
};

int currentScreen = 0; // 0 = folder, 1 = song list, 2 = player, 3 = settings, 4 = keyboard
int tempCurrentScreen = 0;

int selectedFolder = 0;
int currentFolder = 0;

int inputMode = 0; // 0 = files, 1 = player, 2 = volume, 3 = keyboard

bool isShowingKeyboard = false;

bool isSleeping = false;

bool isPlaying = false;

bool isShuffle = false;

bool isMuted = false;

bool isError = false;

int xPos = 10;

String errorMsg = "";

String lastVolume = "";

String searchQuery = "";

String currentEqualizer = "";

unsigned long startTime = 0;
unsigned long currentElapsed = 0;
unsigned long pausedTime = 0;  

unsigned long scrollTime = 0;

unsigned int pausedProgressBarWidth = 0;

unsigned int progressBarWidth = 0;

u8g2_uint_t nameLength;
String currentSongName;
String scrollSongName;
int scrollIndex = 0;
int scrollSpeed = 500;

const int MENU_ITEM_COUNT = 3;

char rootDir[MENU_ITEM_COUNT] [50] = { 
  {"English"}, 
  {"Mixed"}, 
  {"Russian"},
};

// i made a nodejs script to automate getting this information about the songs into a array

const int FOLDER_01_SONGS_COUNT = 2;

char folder01Songs[FOLDER_01_SONGS_COUNT] [50] = { 
  {"artist - song name"}, {"artist - song name"}
};

char folder01SongsDuration[FOLDER_01_SONGS_COUNT] [20] = {
  {"226152"}, {"223536"}
};

const int FOLDER_02_SONGS_COUNT = 2;

char folder02Songs[FOLDER_02_SONGS_COUNT] [80] = { 
  {"artist - song name"}, {"artist - song name"}
};

char folder02SongsDuration[FOLDER_02_SONGS_COUNT] [20] = {
  {"226152"}, {"223536"}
};

const int FOLDER_03_SONGS_COUNT = 2;

char folder03Songs[FOLDER_03_SONGS_COUNT] [80] = { 
  {"artist - song name"}, {"artist - song name"}
};

char folder03SongsDuration[FOLDER_03_SONGS_COUNT] [20] = {
  {"226152"}, {"223536"}
};

void setup() {
  Serial.begin(9600);
  //Serial1.begin(9600);
  Serial.println("Starting");

  Wire.begin(22, 21);
  lipo.enableDebugging();

  if (lipo.begin() == false) // Connect to the MAX17043 using the default wire port
  {
    while (1)
      ;
  }

	lipo.quickStart();

	// We can set an interrupt to alert when the battery SoC gets too low.
	// We can alert at anywhere between 1% - 32%:
	lipo.setThreshold(20); // Set alert threshold to 20%.
  

  pinMode(BUTTON_PIN_NEXT, INPUT_PULLUP);
  pinMode(BUTTON_PIN_BACK, INPUT_PULLUP);
  pinMode(BUTTON_PIN_ENTER, INPUT_PULLUP);
  pinMode(BUTTON_PIN_DIR_BACK, INPUT_PULLUP);
  pinMode(BUTTON_PIN_MODE, INPUT_PULLUP);

  u8g2.begin();
  u8g2.setContrast(0);
  u8g2.enableUTF8Print();

  Serial2.begin(9600);

  delay(500);

  Serial.println("Select storage device to TF card.");
  SendCommand(CMD_SEL_DEV, 0, DEV_TF);
  delay(500); 
  
  //printHelp();

  SendCommand(CMD_SET_VOLUME, 0, 15);

  delay(500); 
  
  SendCommand(CMD_QUERY_VOLUME);
  
  SendCommand(CMD_SET_EQUALIZER, 0, 0x05);
}

void loop() {
  if(digitalRead(BUTTON_PIN_MODE) == LOW){

        if(inputMode == 3){
          if(selectedKeyboard == 0){
            HandleKey(keyboardKeysEnglish[selectedKey][0]);  
          } else if(selectedKeyboard == 1){
            HandleKey(keyboardKeysRussian[selectedKey][0]);  
          }
        }

        if(!isShowingKeyboard){
          inputMode = inputMode + 1;
          if (inputMode >= MODE_ITEM_COUNT) { 
            inputMode = 0;
          }
        }

        if(inputMode == 3 && !isShowingKeyboard){
          isShowingKeyboard = true;
          tempCurrentScreen = currentScreen;

          if(selectedKeyboard == 0){
            selectedKey = ENG_KEYBOARD_LENGHT -1;
          } else if(selectedKeyboard == 1){
            selectedKey = RUS_KEYBOARD_LENGHT -1;
          }

          currentScreen = 4;
        }

        while(digitalRead(BUTTON_PIN_MODE) == LOW){

        }
      }

  if(inputMode == 0){ //file explorer

    if(digitalRead(BUTTON_PIN_ENTER) == LOW){

        Serial.println(selectedFolder);
        if(currentScreen == 1){
          isShuffle = false;
          PlayNext(selectedIndex, CURRENT);
                                        
          Serial.println(selectedIndex);
          currentScreen = 2;
        }

        if(currentScreen == 0){
          selectedFolder = selectedIndex;
          currentScreen = 1;
        }

        if(currentScreen == 3 && selectedMonitorOption == 1){
          SendCommand(CMD_RESET);    
        }

        if(currentScreen == 3 && selectedMonitorOption == 0){
          SendCommand(CMD_QUERY_VOLUME);
          SendCommand(CMD_QUERY_EQUALIZER);
        }
  
        while(digitalRead(BUTTON_PIN_ENTER) == LOW){
          
        }

    }

    if(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){

        if(currentScreen == 3)
        {
          currentScreen = 1;
        }

        if(currentScreen == 0)
        {
          currentScreen = 3;
        }

        if(currentScreen == 1)
        {
          selectedFolder = 0;
          selectedIndex = 0;
          currentScreen--;
        }

        if(currentScreen == 2)
        {
          currentScreen--;
        }

        while(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){
          
        }

    }

    if(digitalRead(BUTTON_PIN_BACK) == LOW) { 

       if(currentScreen == 3){
          selectedMonitorOption = !selectedMonitorOption;
        }

        if(currentScreen != 2){
          selectedIndex = selectedIndex - 1; 

          if (selectedIndex < 0) { 
              if(currentScreen == 0){
                selectedIndex = MENU_ITEM_COUNT-1;
            } else if(currentScreen == 1){
                if(selectedFolder == 0) {
                  selectedIndex = FOLDER_01_SONGS_COUNT-1;
                } else if(selectedFolder == 1){
                  selectedIndex = FOLDER_02_SONGS_COUNT-1;
                } else if(selectedFolder == 2){
                  selectedIndex = FOLDER_03_SONGS_COUNT-1;
                }
            }
          }
        }

        while(digitalRead(BUTTON_PIN_BACK) == LOW){
          
        }
      }

      if(digitalRead(BUTTON_PIN_NEXT) == LOW) {
        if(currentScreen != 2){
          selectedIndex = selectedIndex + 1;
        }
       if(currentScreen == 0){
         if (selectedIndex >= MENU_ITEM_COUNT) { 
            selectedIndex = 0;
          }
       } else if(currentScreen == 1){
          if(selectedFolder == 0) {
              if(selectedIndex >= FOLDER_01_SONGS_COUNT) { 
                selectedIndex = 0;
            }
          } else if(selectedFolder == 1){
               if(selectedIndex >= FOLDER_02_SONGS_COUNT) { 
                selectedIndex = 0;
              }   
          } else if(selectedFolder == 2){
               if(selectedIndex >= FOLDER_03_SONGS_COUNT) { 
                selectedIndex = 0;
              }   
          }
       }

        while(digitalRead(BUTTON_PIN_NEXT) == LOW){
          
        }
      }  

    if(currentScreen == 0){
      previousSelectedIndex = selectedIndex - 1;
      if(previousSelectedIndex < 0) {previousSelectedIndex = MENU_ITEM_COUNT - 1;} 
      nextSelectedIndex = selectedIndex + 1;  
      if(nextSelectedIndex >= MENU_ITEM_COUNT) {nextSelectedIndex = 0;} 
    } else if(currentScreen == 1 && selectedFolder == 0){
        previousSelectedIndex = selectedIndex - 1;
        if(previousSelectedIndex < 0) {previousSelectedIndex = FOLDER_01_SONGS_COUNT - 1;} 
          nextSelectedIndex = selectedIndex + 1;  
          next2SelectedIndex = nextSelectedIndex + 1;
        if(nextSelectedIndex >= FOLDER_01_SONGS_COUNT) {nextSelectedIndex = 0;} 
        if(next2SelectedIndex >= FOLDER_01_SONGS_COUNT) {next2SelectedIndex = 1;} 
    } else if(currentScreen == 1 && selectedFolder == 1){
       previousSelectedIndex = selectedIndex - 1;
        if(previousSelectedIndex < 0) {previousSelectedIndex = FOLDER_02_SONGS_COUNT - 1;} 
          nextSelectedIndex = selectedIndex + 1;  
          next2SelectedIndex = nextSelectedIndex + 1;
        if(nextSelectedIndex >= FOLDER_02_SONGS_COUNT) {nextSelectedIndex = 0;} 
        if(next2SelectedIndex >= FOLDER_02_SONGS_COUNT) {next2SelectedIndex = 1;} 
    } else if(currentScreen == 1 && selectedFolder == 2){
       previousSelectedIndex = selectedIndex - 1;
        if(previousSelectedIndex < 0) {previousSelectedIndex = FOLDER_03_SONGS_COUNT - 1;} 
          nextSelectedIndex = selectedIndex + 1;  
          next2SelectedIndex = nextSelectedIndex + 1;
        if(nextSelectedIndex >= FOLDER_03_SONGS_COUNT) {nextSelectedIndex = 0;} 
        if(next2SelectedIndex >= FOLDER_03_SONGS_COUNT) {next2SelectedIndex = 1;} 
    }

  } else if(inputMode == 1){ //player mode
      if(currentScreen == 1 || currentScreen == 2){
		    if(digitalRead(BUTTON_PIN_NEXT) == LOW){
        
        	PlayNext(selectedIndex, FORWARD);
  
        	while(digitalRead(BUTTON_PIN_NEXT) == LOW){

        	}
    	} 

    	if(digitalRead(BUTTON_PIN_BACK) == LOW){
    
        	PlayNext(selectedIndex, BACK);

        	while(digitalRead(BUTTON_PIN_BACK) == LOW){

      	}
    	} 
    }

    if(digitalRead(BUTTON_PIN_ENTER) == LOW){
    
        // play&pause

        isPlaying = !isPlaying;

        if(!isPlaying){
          Serial.println("Pause");
          SendCommand(CMD_PAUSE);

          pausedTime = currentElapsed;  
          pausedProgressBarWidth = progressBarWidth;
          isPlaying = false; 

        } else {
          Serial.println("Play");
          SendCommand(CMD_PLAY);

          startTime = millis(); 
          isPlaying = true;  
        }
  
        while(digitalRead(BUTTON_PIN_ENTER) == LOW){
          
        }

    }

    if(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){
    
        // shuffle

        isShuffle = !isShuffle;

        if(isShuffle){
          currentFolder = selectedFolder;
        }
  
        while(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){
          
        }

    }
  } else if(inputMode == 2){
    if(digitalRead(BUTTON_PIN_NEXT) == LOW){
      Serial.println("Volume Down");  
      
      delay(100);

      prevVolume = lastVolume.toInt();
      
      int calcVolume = prevVolume - volumeTick;
      
      
      if(calcVolume > 0){
        SendCommand(CMD_SET_VOLUME, 0, calcVolume);
      } else {
        SendCommand(CMD_SET_VOLUME, 0, 0);
      }
      delay(100);
      SendCommand(CMD_QUERY_VOLUME);

      while(digitalRead(BUTTON_PIN_NEXT) == LOW){

      }
  } 

    if(digitalRead(BUTTON_PIN_BACK) == LOW){
      Serial.println("Volume Up");  
      
      delay(100);
      
      prevVolume = lastVolume.toInt();
      
      
      int calcVolume = prevVolume + volumeTick;
      
      if(calcVolume < 30){
        SendCommand(CMD_SET_VOLUME, 0, calcVolume);
      } else {
        SendCommand(CMD_SET_VOLUME, 0, 30);
      }
      delay(100);
      SendCommand(CMD_QUERY_VOLUME);

      while(digitalRead(BUTTON_PIN_BACK) == LOW){

      }
    } 

    if(digitalRead(BUTTON_PIN_ENTER) == LOW){
    
        SendCommand(CMD_QUERY_VOLUME);

        if(!isMuted){
          prevVolume = lastVolume.toInt();
        }

        isMuted = !isMuted;

        if(isMuted){
          SendCommand(CMD_SET_VOLUME, 0, 0);
        } else {
          //SendCommand(CMD_SET_VOLUME, 0, volume);
          SendCommand(CMD_SET_VOLUME, 0, prevVolume);
        }

        while(digitalRead(BUTTON_PIN_ENTER) == LOW){
          
        }
    }

    if(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){
    
        if(volumeMode >= 2){
          volumeMode = 0;
        } else {
          volumeMode++;
        }

        if(volumeMode == 0){
          volumeTick = 1;
        } else if(volumeMode == 1) {
          volumeTick = 5;
        } else if(volumeMode == 2) {
          volumeTick = 10;
        }

        Serial.println(volumeMode);

        while(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){
          
        }

    }
  } else if(inputMode == 3){ // keyboard
      if(digitalRead(BUTTON_PIN_ENTER) == LOW){

      if(selectedKeyboard == 0){
        if(selectedKey + 1 <= ENG_KEYBOARD_LENGHT - 1){ // m[ske fjern minus 1
          selectedKey+=1;
        }     
      } else if(selectedKeyboard == 1){
        if(selectedKey + 1 <= RUS_KEYBOARD_LENGHT - 1){
          selectedKey+=1;
        }
      }

      while(digitalRead(BUTTON_PIN_ENTER) == LOW){

      }
    } 

    if(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){
      if(selectedKeyboard == 0){
          if(selectedKey - 1 >= 0){
            selectedKey-=1;
          }     
        } else if(selectedKeyboard == 1){
          if(selectedKey - 1 >= 0){
            selectedKey-=1;
          }
      }
      
      while(digitalRead(BUTTON_PIN_DIR_BACK) == LOW){

      }
    } 

    if(digitalRead(BUTTON_PIN_BACK) == LOW){
    
        if(selectedKeyboard == 0){
          if(selectedKey - 10 >= 0){
            selectedKey-=10;
          }
        } else if(selectedKeyboard == 1){
          if(selectedKey - 10 >= 0){
            selectedKey-=10;
          }
        }

        while(digitalRead(BUTTON_PIN_BACK) == LOW){
          
        }
    }

    if(digitalRead(BUTTON_PIN_NEXT) == LOW){
    
      if(selectedKeyboard == 0){
        if(selectedKey + 10 <= ENG_KEYBOARD_LENGHT){
          selectedKey+=10;
        } else {
          selectedKey = ENG_KEYBOARD_LENGHT - 1;
        }
      } else if(selectedKeyboard == 1){
        if(selectedKey + 10 <= RUS_KEYBOARD_LENGHT){
          selectedKey+=10;
        } else {
          selectedKey = RUS_KEYBOARD_LENGHT - 1;
        }
      }

        while(digitalRead(BUTTON_PIN_NEXT) == LOW){
          
        }

    }
  }

  u8g2.firstPage();
   do  {   

    // status bar
        //battery
        u8g2.setFont(u8g2_font_5x8_t_cyrillic);
        u8g2.setCursor(0, 7);
        u8g2.print(lipo.getSOC(), 0); 
        u8g2.print("%");

        if(lipo.getVoltage() >= 4.14 && lipo.getSOC() < 100.00){
          u8g2.setFont(u8g2_font_open_iconic_other_1x_t);
          u8g2.print("\u0040");
        }

        //inputmode
        u8g2.setFont(u8g2_font_5x8_t_cyrillic);
        u8g2.setCursor(23, 7); 
        u8g2.print("Mode:");
        u8g2.setCursor(50, 7); 
        if(inputMode == 0){
          u8g2.print("Files");
        } else if(inputMode == 1){
          u8g2.print("Player");
        } else if(inputMode == 2){
          u8g2.print("Volume");
        } else if(inputMode == 3){
          u8g2.print("Search");
        }

        //volume
        u8g2.setFont(u8g2_font_open_iconic_play_1x_t);
        u8g2.setCursor(82, 8); 
        u8g2.print("\u0050");

        u8g2.setFont(u8g2_font_5x8_t_cyrillic);
        u8g2.setCursor(92, 7); 
        u8g2.print(lastVolume);

        //error
        if(isError){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(102, 7); 
          u8g2.print("!");
        }
        
        //muted on/off
        if(isMuted){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(108, 7); 
          u8g2.print("M");
        }

        //play on/off
        if(!isPlaying){
          u8g2.setFont(u8g2_font_open_iconic_play_1x_t);
          u8g2.setCursor(114, 8); 
          u8g2.print("\u0044");
        }

        //shuffle on/off
        if(isShuffle){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(124, 7); 
          u8g2.print("S");
        }

        u8g2.drawLine(0, 9, 128, 9);
        //status bar

      if(currentScreen == 3){
        u8g2.setFont(u8g2_font_7x13_t_cyrillic);
        u8g2.setCursor(DISPLAY_WIDTH / 2 - 20, 21); 
        u8g2.print("Monitor");

        if(isError){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 24); 
          u8g2.print("Error:");
          u8g2.setCursor(0, 32); 
          u8g2.print(errorMsg);
        } 

        u8g2.setFont(u8g2_font_5x8_t_cyrillic);
        u8g2.setCursor(0, 40); 
        u8g2.print("Voltage: ");
        u8g2.print(lipo.getVoltage());

        u8g2.setCursor(0, 48); 
        u8g2.print("Battery: ");
        u8g2.print(lipo.getSOC());
        u8g2.print("%");

        u8g2.setCursor(DISPLAY_WIDTH - 50, 40); 
        u8g2.print("Songs:");
        u8g2.print(FOLDER_01_SONGS_COUNT + FOLDER_02_SONGS_COUNT + FOLDER_03_SONGS_COUNT);

        u8g2.setCursor(DISPLAY_WIDTH - 40, 48); 
        u8g2.print("Eq:");
        u8g2.print(currentEqualizer);

        if(selectedMonitorOption == 0){
          u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
          u8g2.setCursor(DISPLAY_WIDTH / 2 - 50, 62); 
          u8g2.print("\u004E");
        }

        u8g2.setFont(u8g2_font_6x12_t_cyrillic);
        u8g2.setCursor(DISPLAY_WIDTH / 2 - 40, 62); 
        u8g2.print("Check");

        if(selectedMonitorOption == 1){
          u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
          u8g2.setCursor(DISPLAY_WIDTH / 2, 62); 
          u8g2.print("\u004E");
        }

        u8g2.setFont(u8g2_font_6x12_t_cyrillic);
        u8g2.setCursor(DISPLAY_WIDTH / 2 + 10, 62); 
        u8g2.print("Reset");
      }

      if(currentScreen == 0){
        u8g2.setFont(u8g2_font_7x13_t_cyrillic);
        u8g2.setCursor(0, 30); 
        u8g2.print(rootDir[previousSelectedIndex]);

        u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
        u8g2.setCursor(0, 45); 
        u8g2.print("\u004E");
        u8g2.setFont(u8g2_font_7x13_t_cyrillic);
        u8g2.setCursor(10, 45); 
        u8g2.print(rootDir[selectedIndex]);

        u8g2.setFont(u8g2_font_7x13_t_cyrillic);
        u8g2.setCursor(0, 60); 
        u8g2.print(rootDir[nextSelectedIndex]);
      }

      if(currentScreen == 1){
        if(selectedFolder == 0){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 20); 
          u8g2.print(folder01Songs[previousSelectedIndex]);

          u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
          u8g2.setCursor(0, 35); 
          u8g2.print("\u004E");
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);

          u8g2.setCursor(10, 35);
          
          
          //check for name length til at fixe hoppet n[r den ikke skal scroll
          nameLength = String(folder01Songs[selectedIndex]).length();

          if(millis() > scrollTime + scrollSpeed){
            scrollSongName = String(folder01Songs[selectedIndex]).substring(scrollIndex, scrollIndex + 24);

            scrollIndex = scrollIndex + 1;

            if(scrollIndex > (nameLength-24)) {
              scrollIndex = 0;
            }
            scrollTime = millis();
          }

          u8g2.print(scrollSongName);

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 50); 
          u8g2.print(folder01Songs[nextSelectedIndex]);

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 60); 
          u8g2.print(folder01Songs[next2SelectedIndex]);
        } else if(selectedFolder == 1){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 20); 
          u8g2.print(folder02Songs[previousSelectedIndex]);

          u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
          u8g2.setCursor(0, 35); 
          u8g2.print("\u004E");
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          
         
          u8g2.setCursor(10, 35); 
          //check for name length til at fixe hoppet n[r den ikke skal scroll
          nameLength = String(folder02Songs[selectedIndex]).length();

          if(millis() > scrollTime + scrollSpeed){
            scrollSongName = String(folder02Songs[selectedIndex]).substring(scrollIndex, scrollIndex + 24);

            scrollIndex = scrollIndex + 1;

            if(scrollIndex > (nameLength-24)) {
              scrollIndex = 0;
            }
            scrollTime = millis();
          }

          u8g2.print(scrollSongName);

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 50); 
          u8g2.print(folder02Songs[nextSelectedIndex]);

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 60); 
          u8g2.print(folder02Songs[next2SelectedIndex]);
        } else if(selectedFolder == 2){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 20); 
          u8g2.print(folder03Songs[previousSelectedIndex]);

          u8g2.setFont(u8g2_font_open_iconic_arrow_1x_t);
          u8g2.setCursor(0, 35); 
          u8g2.print("\u004E");
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);

          u8g2.setCursor(10, 35); 
          //check for name length til at fixe hoppet n[r den ikke skal scroll
          nameLength = String(folder03Songs[selectedIndex]).length();

          if(millis() > scrollTime + scrollSpeed){
            scrollSongName = String(folder03Songs[selectedIndex]).substring(scrollIndex, scrollIndex + 35);

            scrollIndex = scrollIndex + 1;

            if(scrollIndex > (nameLength-35)) {
              scrollIndex = 0;
            }
            scrollTime = millis();
          }

          u8g2.print(scrollSongName);

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 50); 
          u8g2.print(folder03Songs[nextSelectedIndex]);

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(0, 60); 
          u8g2.print(folder03Songs[next2SelectedIndex]);
        }
        
      }

      if(currentScreen == 4){

        if(selectedFolder == 0){
          u8g2.setFont(u8g2_font_4x6_t_cyrillic);
          u8g2.setCursor(0, 18); 
          u8g2.print("eng");
        } else if(selectedFolder == 1){
          u8g2.setFont(u8g2_font_4x6_t_cyrillic);
          u8g2.setCursor(0, 18); 
          u8g2.print("mix");
        } else if(selectedFolder == 2){
          u8g2.setFont(u8g2_font_4x6_t_cyrillic);
          u8g2.setCursor(0, 18); 
          u8g2.print("rus");
        }

        if(selectedKeyboard == 0){
          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(55, 18); 
          u8g2.print(searchQuery);

          u8g2.setFont(u8g2_font_7x13_t_cyrillic);

          int spacing = -5;
          int PADDING_AMOUNT = 12;

          for (size_t i = 0; i < 10; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 28); 
            u8g2.print(keyboardKeysEnglish[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 36);
              u8g2.print("-");
            }
          }  
          spacing = -5;
          for (size_t i = 10; i < 20; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 42); 
            u8g2.print(keyboardKeysEnglish[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 51);
              u8g2.print("-");
            }
          } 

          spacing = -5;
          for (size_t i = 20; i < ENG_KEYBOARD_LENGHT; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 57); 
            u8g2.print(keyboardKeysEnglish[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 64);
              u8g2.print("-");
            }
          } 
        } else if(selectedKeyboard == 1){

          u8g2.setFont(u8g2_font_5x8_t_cyrillic);
          u8g2.setCursor(55, 18); 
          u8g2.print(searchQuery);
          u8g2.setFont(u8g2_font_6x12_t_cyrillic);

          int spacing = -5;
          int PADDING_AMOUNT = 12;

          for (size_t i = 0; i < 10; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 26); 
            u8g2.print(keyboardKeysRussian[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 33);
              u8g2.print("-");
            }
          }
          spacing = -5;
          for (size_t i = 10; i < 20; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 37); 
            u8g2.print(keyboardKeysRussian[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 43);
              u8g2.print("-");
            }
          } 

          spacing = -5;
          for (size_t i = 20; i < 30; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 47); 
            u8g2.print(keyboardKeysRussian[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 53);
              u8g2.print("-");
            }
          } 
          spacing = -5;
          for (size_t i = 30; i < RUS_KEYBOARD_LENGHT; i++)
          {
            spacing = spacing + PADDING_AMOUNT;
            u8g2.setCursor(spacing, 58); 
            u8g2.print(keyboardKeysRussian[i]);

            if(selectedKey == i){
              u8g2.setCursor(spacing, 64);
              u8g2.print("-");
            }
          }
        }

      }

      if(currentScreen == 2){
        u8g2.setFont(u8g2_font_7x13_t_cyrillic);
        u8g2.setCursor(0, 20); 
        u8g2.print("Now playing:");

        u8g2.setFont(u8g2_font_4x6_t_cyrillic);
        
        u8g2.setCursor(0, 30); 

        nameLength = currentSongName.length();

        //check for name length til at fixe hoppet n[r den ikke skal scroll

        if(millis() > scrollTime + scrollSpeed){
          scrollSongName = currentSongName.substring(scrollIndex, scrollIndex + 32);
                
          scrollIndex = scrollIndex + 1;
            
          if(scrollIndex > (nameLength-32)) {
            scrollIndex = 0;
          }
          scrollTime = millis();
        }
              
        u8g2.print(scrollSongName);

        u8g2.setFont(u8g2_font_4x6_t_cyrillic);
        u8g2.setCursor(DISPLAY_WIDTH / 2 - 24, 49); 
        if(selectedFolder == 0){
          if(isPlaying){

            HandleTime(folder01SongsDuration[selectedIndex]);

          } else {
            u8g2.print(FormatTimeMs(currentElapsed));
            u8g2.print(" / ");
            u8g2.print(FormatTime(folder01SongsDuration[selectedIndex])); 
            u8g2.drawBox(2, 56, progressBarWidth, 6); 
          }
        } else if(selectedFolder == 1){
          if(isPlaying){
            
            HandleTime(folder02SongsDuration[selectedIndex]);

          } else {
            u8g2.print(FormatTimeMs(currentElapsed));
            u8g2.print(" / ");
            u8g2.print(FormatTime(folder02SongsDuration[selectedIndex])); 
            u8g2.drawBox(2, 56, progressBarWidth, 6);  
          }
        } else if(selectedFolder == 2){
          if(isPlaying){
            
            HandleTime(folder03SongsDuration[selectedIndex]);

          } else {
            u8g2.print(FormatTimeMs(currentElapsed));
            u8g2.print(" / ");
            u8g2.print(FormatTime(folder03SongsDuration[selectedIndex]));  
            u8g2.drawBox(2, 56, progressBarWidth, 6); 
          }
        }       

        u8g2.drawFrame(0, 54, DISPLAY_WIDTH, 10);
      }

   }  while(u8g2.nextPage());{
        delay(100);
   }

  char c = ' ';

  if(Serial.available() ){
    c = Serial.read();
    SendMP3Command(c);
  } 

  if(Serial2.available()){
    Serial.println(decodeMP3Answer());
  }
  
  delay(10);
}

void HandleKey(char c){

  Serial.println(c);

  if(selectedKeyboard == 0){
      String englishAlphabet = "abcdefghijklmnopqrstuvwxyz";

      if(englishAlphabet.indexOf(c) != -1) {
        searchQuery += c; 
      }

  } else if(selectedKeyboard == 1){  
      String cyrillicAlphabet = "абвгдеёжзийклмнопрстуфхцчшщъыьэюя";

      if(cyrillicAlphabet.indexOf(c) != -1) {
        searchQuery += c; 
      }
  }

  //if(c.indexOf('^')) {
  //    if(selectedKeyboard == 0){
  //      selectedKeyboard = 1;
  //      selectedKey = 33;
  //    } else if(selectedKeyboard == 1){
  //      selectedKeyboard = 0;
  //      selectedKey = 26;
  //    }
  //} else if(c.indexOf('<')) {
  //    if(searchQuery.length() > 0){
  //      searchQuery.remove(searchQuery.length() - 1);
  //    }
  //} else if(c.indexOf('>')) {
  //    if(!searchQuery.isEmpty()){
  //      Search(searchQuery);
  //    }
  //} else if(c.indexOf('X')) {
  //    currentScreen = tempCurrentScreen;
  //    isShowingKeyboard = false;
  //}

  if(c == '^') {
      if(selectedKeyboard == 0){
        selectedKeyboard = 1;
        selectedKey = 33;
      } else if(selectedKeyboard == 1){
        selectedKeyboard = 0;
        selectedKey = 26;
      }
  } else if(c == '<') {
      if(searchQuery.length() > 0){
        searchQuery.remove(searchQuery.length() - 1);
      }
  } else if(c == '>') {
      if(!searchQuery.isEmpty()){
        Search(searchQuery);
      }
  } else if(c == 'X') {
      currentScreen = tempCurrentScreen;
      isShowingKeyboard = false;
  }
}

void Search(String query){

  Serial.println(query);

  if(selectedFolder == 0){
    for (size_t i = 0; i < FOLDER_01_SONGS_COUNT; i++)
    {
      String songName = folder01Songs[i];

      String songNameLower = songName;
      songNameLower.toLowerCase();

      if(songNameLower.indexOf(query) != -1){
        selectedIndex = i;
        break;
      }
    }
  } else if(selectedFolder == 1){
    for(size_t i = 0; i < FOLDER_02_SONGS_COUNT; i++)
    {
      String songName = folder02Songs[i];

      String songNameLower = songName;
      songNameLower.toLowerCase();

      if(songNameLower.indexOf(query) != -1){
        selectedIndex = i;
        break;
      }
    }
  } else if(selectedFolder == 2){
    for(size_t i = 0; i < FOLDER_03_SONGS_COUNT; i++)
    {
      String songName = folder03Songs[i];

      String songNameLower = songName;
      songNameLower.toLowerCase();

      if(songNameLower.indexOf(query) != -1){
        selectedIndex = i;
        break;
      }
    }
  }

  currentScreen = 1;

  isShowingKeyboard = false;
  
  inputMode = 3;

  searchQuery = "";
}

String FormatTimeMs(unsigned long ms) {
    unsigned int minutes = ms / 60000;
    unsigned int seconds = (ms % 60000) / 1000;

    String timeString = "";
    if(minutes < 10) {
      timeString += "0";
    }
    timeString += String(minutes) + ":";
    if(seconds < 10) {
      timeString += "0";
    }
    timeString += String(seconds);

    return timeString;
}

String FormatTime(const char *durationMs) {
    unsigned long ms = atol(durationMs);
    unsigned int minutes = ms / 60000;
    unsigned int seconds = (ms % 60000) / 1000;

    String timeString = "";
    if(minutes < 10) {
      timeString += "0";
    }
    timeString += String(minutes) + ":";
    if(seconds < 10) {
      timeString += "0";
    }
    timeString += String(seconds);

    return timeString;
}

void HandleTime(char* duration){
  currentElapsed = millis() - startTime + pausedTime;

  unsigned long songDuration = atol(duration);

  progressBarWidth = (currentElapsed * (DISPLAY_WIDTH - 4)) / atol(duration);

  if(currentElapsed >= songDuration) {
    currentElapsed = songDuration; 
    
  }

  u8g2.print(FormatTimeMs(currentElapsed));
  u8g2.print(" / ");
  u8g2.print(FormatTime(duration)); 
  u8g2.drawBox(2, 56, progressBarWidth, 6);
}

void PlayNext(int currentSong, int direction){
  int nextSong;

  //SendCommand(CMD_SET_EQUALIZER, 0, 0x05);

  if(isShuffle){
    if(direction == 1){
      if(currentFolder == 0) {
        nextSong = random(FOLDER_01_SONGS_COUNT);
      } else if(currentFolder == 1){
        nextSong = random(FOLDER_02_SONGS_COUNT);
      } else if(currentFolder == 2){
        nextSong = random(FOLDER_03_SONGS_COUNT);
      }
    } else if (direction == 0){
      nextSong = currentSong - 1;
    }
  } else {
    if(direction == 1){
      nextSong = currentSong + 1;
    } else if (direction == 0){
      nextSong = currentSong - 1;
    } else if (direction == 2){
      nextSong = currentSong;
      currentFolder = selectedFolder;
    }
  }

  selectedIndex = nextSong;

  if(currentFolder == 0){
    if (nextSong < 0) {
      nextSong = FOLDER_01_SONGS_COUNT-1;
      selectedIndex = FOLDER_01_SONGS_COUNT-1;
    }   

    if(nextSong >= FOLDER_01_SONGS_COUNT){
      nextSong = 0;
      selectedIndex = 0;
    } 
    currentSongName = folder01Songs[selectedIndex];
  } else if(currentFolder == 1){
    if (nextSong < 0) {
      nextSong = FOLDER_02_SONGS_COUNT-1;
      selectedIndex = FOLDER_02_SONGS_COUNT-1;
    }   

    if(nextSong >= FOLDER_02_SONGS_COUNT){
      nextSong = 0;
      selectedIndex = 0;
    } 
    currentSongName = folder02Songs[selectedIndex];
  } else if(currentFolder == 2){
    if (nextSong < 0) {
      nextSong = FOLDER_03_SONGS_COUNT-1;
      selectedIndex = FOLDER_03_SONGS_COUNT-1;
    } 

    if(nextSong >= FOLDER_03_SONGS_COUNT){
      nextSong = 0;
      selectedIndex = 0;
    }
    
    currentSongName = folder03Songs[selectedIndex];
  }

  SendCommand(CMD_PLAY_FOLDER_FILE, currentFolder + 1, nextSong + 1);

  Serial.println(currentSongName);

  currentElapsed = 0; 
  pausedTime = 0;      
  startTime = millis();
  isPlaying = true;
  progressBarWidth = 0;
}

void SendMP3Command(char c) {
  switch (c) {

    case 'Q':
      SendCommand(CMD_QUERY_EQUALIZER);
    break;

    case 'p':
      Serial.println("Play ");
      SendCommand(CMD_PLAY);
      break;

    case 'P':
      Serial.println("Pause");
      SendCommand(CMD_PAUSE);
      break;

    case '>':
      Serial.println("Next");
      PlayNext(selectedIndex, FORWARD);
      break;

    case '<':
      Serial.println("Previous");
      PlayNext(selectedIndex, BACK);
      break;

    case 's':
      Serial.println("Stop Play");
      SendCommand(CMD_STOP_PLAY);
      break;

    case '+':
      Serial.println("Volume Up");
      SendCommand(CMD_VOLUME_UP);
      break;

    case '-':
      Serial.println("Volume Down");
      SendCommand(CMD_VOLUME_DOWN);
      break;

    case 'c':
      Serial.println("Query current file");
      SendCommand(CMD_PLAYING_N);
      break;

    case 'q':
      Serial.println("Query status");
      SendCommand(CMD_QUERY_STATUS);
      break;

    case 'v':
      Serial.println("Query volume");
      SendCommand(CMD_QUERY_VOLUME);
      break;

    case 'x':
      Serial.println("Query folder count");
      SendCommand(CMD_QUERY_FLDR_COUNT);
      break;

    case 't':
      Serial.println("Query total file count");
      SendCommand(CMD_QUERY_TOT_TRACKS);
      break;

    case 'f':
      Serial.println("Playing folder 1");
      SendCommand(CMD_FOLDER_CYCLE, 0x00, 0x01);
                                         // ^ folder
      break;

    case 'S':
      Serial.println("Sleep");
      SendCommand(CMD_SLEEP_MODE);
      break;

    case 'W':
      Serial.println("Wake up");
      SendCommand(CMD_WAKE_UP);
      break;

    case 'r':
      Serial.println("Reset");
      SendCommand(CMD_RESET);
      break;
  }
}

String decodeMP3Answer() {
  String decodedMP3Answer = "";

  decodedMP3Answer += sanswer();

  switch (ansbuf[3]) {
    case 0x3b:
      decodedMP3Answer += " -> Memory card removed.";
      errorMsg = "Memory card removed";
      isError = true;
      break;

    case 0x3A:
      decodedMP3Answer += " -> Memory card inserted.";
      isError = false;
      break;

    case 0x3D:
      decodedMP3Answer += " -> Completed play num " + String(ansbuf[6], DEC);
      PlayNext(String(ansbuf[6], DEC).toInt(), 1);
      break;

    case 0x40:
      decodedMP3Answer += " -> Error";
      errorMsg = "Unknown error 0x40";
      isError = true;
      break;

    case 0x41:
      decodedMP3Answer += " -> Data recived correctly.";
      isError = false;
      break;

    case 0x42:
      decodedMP3Answer += " -> Status playing: " + String(ansbuf[6], DEC);
      isError = false;
      break;

    case 0x43:
      lastVolume = String(ansbuf[6], DEC);
      decodedMP3Answer += " -> Volume level: " + String(ansbuf[6], DEC);
      isError = false;
      break;

    case 0x44:
      currentEqualizer = String(ansbuf[6], DEC).toInt();
      decodedMP3Answer += " -> Equalizer status: " + String(ansbuf[6], DEC);
      break;

    case 0x48:
      decodedMP3Answer += " -> File count: " + String(ansbuf[6], DEC);
      break;

    case 0x4C:
      decodedMP3Answer += " -> Playing: " + String(ansbuf[6], DEC);
      break;

    case 0x4E:
      decodedMP3Answer += " -> Folder file count: " + String(ansbuf[6], DEC);
      break;

    case 0x4F:
      decodedMP3Answer += " -> Folder count: " + String(ansbuf[6], DEC);
      break;
  }

  return decodedMP3Answer;
}

void SendCommand(byte command){
  SendCommand(command, 0, 0);
}

void SendCommand(byte command, byte dat1, byte dat2){
  delay(20);
  Send_buf[0] = 0x7E;    //-> Every command should start with $(0x7E)
  Send_buf[1] = 0xFF;    //-> Version information
  Send_buf[2] = 0x06;    //-> The number of bytes of the command without starting byte and ending byte
  Send_buf[3] = command; //-> Such as PLAY and PAUSE and so on
  Send_buf[4] = 0x01;    //-> 0x00 = not feedback, 0x01 = feedback
  Send_buf[5] = dat1;    //-> data1
  Send_buf[6] = dat2;    //-> data2
  Send_buf[7] = 0xEF;    //-> Ending byte of the command
  
  Serial.print("Sending: ");
  
  for(uint8_t i = 0; i < 8; i++){
    Serial2.write(Send_buf[i]);
    Serial.print(sbyte2hex(Send_buf[i]));
  }
  
  Serial.println();
}

String sbyte2hex(uint8_t b){
  String shex;

  shex = "0X";

  if (b < 16) shex += "0";
  shex += String(b, HEX);
  shex += " ";
  return shex;
}

int shex2int(char *s, int n){
  int r = 0;
  for (int i=0; i<n; i++){
     if(s[i]>='0' && s[i]<='9'){
      r *= 16; 
      r +=s[i]-'0';
     }else if(s[i]>='A' && s[i]<='F'){
      r *= 16;
      r += (s[i] - 'A') + 10;
     }
  }
  return r;
}

String sanswer(void){
  uint8_t i = 0;
  String mp3answer = ""; 

  while(Serial2.available() && (i < 10)){
    uint8_t b = Serial2.read();
    ansbuf[i] = b;
    i++;

    mp3answer += sbyte2hex(b);
  }

  if((ansbuf[0] == 0x7E) && (ansbuf[9] == 0xEF)){
    return mp3answer;
  }

  isError = true;
  errorMsg = "MP3 connection failed";

  return "???: " + mp3answer;
}
