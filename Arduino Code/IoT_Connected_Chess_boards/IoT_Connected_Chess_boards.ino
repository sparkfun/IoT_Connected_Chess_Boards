/* IoT Connected Chess boards
 *  
 * This code was written for the ESP8266 SparkFun Thing Dev Board
 * on Arduino 1.6.7
 * 8/1/2016
 * By Sarah Al-Mutlaq
 * SparkFun Electronics
 * 
 * This code is used in both connected Chess boards project
 * from the Fellowship of the Things episode 7 video
 * 
 * This project allows you to have two connected Chess boards 
 * that communicate over Phant (data.sparkfun.com) so that you 
 * can play chess on a physical board with the other board,
 * wherever, over WiFi.
 * 
 * This code keeps track of the chess pieces, looks for a move
 * if it is your turn or waits until it is your turn. It sends 
 * and recieves it's data over Phant. 
 * 
 * This code is the same for both boards, so it is all the code 
 * you need, because you can see in the code that there is 
 * a case for what board, and all the code checks which board 
 * you are using.
 *

 * Pins connected are as follows:
 * 
 * SparkFun ESP8266 Thing Dev Board are as follows:
 * Pin 0: clockpin on shift in register
 * Pin 2: ploadpin on shift in register
 * Pin 5: Din of WS2812 string
 * Pin 4: dataPin on shift in register
 * Pin 12: TX of LCD screen
 * Pin 13: clock enable pin of shift in register
 * Pin 14: Side switch
 * GND: Ground
 * VIN: High
 * 
 * LCD and Shift-in registers all connected to high and ground.
 * Buttons and reed switches are pulled high.
 * LEDs have thier own power not from the Thing Dev board (draws too much current)
 * Use a 1000microF capacitior between LED High and Ground
 * Use a 330 ohm resistor between LED Din and Pin 5 of the Thing Dev board
 * 
 * 
 *Numbers for pieces:   
 * White:                 Color they show up on the board as:
 * [R]ooks =    1 and 2   (Red)
 * k[N]ights =  3 and 4   (Green)
 * [B]ishops =  5 and 6   (Blue)
 * [Q]ueen =    7         (Purple)
 * [K]ing =     8         (Gold) 
 * [ ]Pawns =   9         (White)
 * 
 * Black:
 * [ ]Pawns =   10        (White)
 * [R]ooks =    11 and 12 (Red)
 * k[N]ights =  13 and 14 (Green)
 * [B]ishops =  15 and 16 (Blue)
 * [Q]ueen =    17        (Purple)
 * [K]ing =     18        (Gold)
 * 
 * 
 * In general in the code 0 means white and 1 means black for keeping track 
*/


//include the library for the WS2812 LEDs or "NeoPixels":
#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    5    // Digital IO pin connected to the NeoPixels.

#define PIXEL_COUNT 64     //Number of pixels (LEDs)

//How to define the NeoPixel strips in the Adafruit library
//I have defined two, one for the black board and one for the white:
Adafruit_NeoPixel whiteStrip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel blackStrip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Use the softwareserial library to create a new "soft" serial port
// for the display. This prevents display corruption when uploading code.
#include <SoftwareSerial.h>

// Attach the serial LCD's RX line to digital pin 12:
SoftwareSerial whiteSerial(3,12); // pin 12 = TX, pin 3 = RX (unused)
SoftwareSerial blackSerial(3,12); // pin 12 = TX, pin 3 = RX (unused)

// How many shift register chips are daisy-chained:
#define NUMBER_OF_SHIFT_CHIPS   9

// Width of shift register data (how many ext lines):
#define DATA_WIDTH   NUMBER_OF_SHIFT_CHIPS * 8

// Width of pulse to trigger the shift register to read and latch:
#define PULSE_WIDTH_USEC   5

// Optional delay between shift register reads:
#define POLL_DELAY_MSEC   1

int ploadPin        = 2;  // Connects to Parallel load pin the 165 shift register
int clockEnablePin  = 13;  // Connects to Clock Enable pin the 165 shift register
int dataPin         = 4; // Connects to the Q7 pin the 165 shift register
int clockPin        = 0; // Connects to the Clock pin the 165 shift register

int boardSidePin = 14; // Connects to board side switch, that is pulled high

// Include the ESP8266 WiFi library. (Works a lot like the
// Arduino WiFi library.)
#include <ESP8266WiFi.h>
// Include WiFiClient library:
#include <WiFiClient.h>
// Include the SparkFun Phant library:
#include <Phant.h>

//Define your WiFi connection: ****(WiFi name and password need to be changed)****
const char WIFI_SSID[] = "sparkfun-guest";
const char WIFI_PSK[] = "sparkfun6333";


// Phant Keys: ****(To make your own page, and get those keys go to data.sparkfun.com)****
const char http_site[] = "data.sparkfun.com";
const char PublicKey[] = "2JYaxdVrEbsrL8maXZJL";
const char PrivateKey[] = "GPnKyY48DACwe9VjaJbe";
const int http_port = 80;



//Array to keep track of all the pieces on the univeral board
long boardStatus[] = {1, 3, 5, 7, 8, 6, 4, 2, 
                     9, 9, 9, 9, 9, 9, 9, 9,
                     0, 0, 0, 0, 0, 0, 0, 0, 
                     0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0, 0, 0, 0,
                     10, 10, 10, 10, 10, 10, 10, 10, 
                     11, 13, 15, 17, 18, 16, 14, 12};
                     

//Array to keep track of the actual magnetic input from the white board
long whiteMagnetInput[] = {0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1};

//Array to keep track of the last magnetic input from the white board
long oldWhiteMagnetInput [] = {0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1};

//Array to keep track of the actual magnetic input from the white board
long blackMagnetInput[] = {1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          1, 1, 1, 1, 1, 1, 1, 1,
                          0, 0, 0, 0, 0, 0, 0, 0,
                          0, 0, 0, 0, 0, 0, 0, 0};

//Array to keep track of the last magnetic input from the white board
long oldBlackMagnetInput [] = {1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              1, 1, 1, 1, 1, 1, 1, 1,
                              0, 0, 0, 0, 0, 0, 0, 0,
                              0, 0, 0, 0, 0, 0, 0, 0};

//Array to keep track of all of the button states, pulled high:
int buttonStates[] = {1,1,1,1,1,1,1,1};

//Variables to keep track of all the different things, very self explanitory:
int physicalBoard;
char turn = 0; //An integer to keep track of whose turn it is 0 = White, 1 = Black
int pieceCodeMoved;  
char pieceMoved;
int boardChange = 0;
int captureCheck = 0;
String moveFrom;
String moveTo;
int moveFromNumber;
int moveToNumber;
String displayWords = "                ";
int noChangeFlag = 1;
int pieceDelay = 2000; //Time you allow players to change their mind about their move in milliseconds
int positionMovedFrom;
int firstPieceMoved;
char col;
int row;
int posted = 0;
String readString;
int startReading;
String postWords;
String postTurn;
String getPosition;
int yourPieceCaptured = 0;
int firstTurn = 1;
int removedPiece = 0;
int updatedLights = 0;
int side;
int pawnMoved = 0;
int pawnPromotionCheck = 0;
int pawnToQueen;
int enPassantCheck = 0;
int castleSide;
int drawRequest = 0;
int buttonNumber;
int pawnPromotionMade = 0;

//define the WiFiClient (as client): 
WiFiClient client;


//This is a function that returns a 0 or 1
//You input two arrays and it checks if they are the same
//1 means they are the same, 0 means they are not.
boolean arraysEqual(long one[DATA_WIDTH], long two[DATA_WIDTH]){
  int flag;
  for (int i = 0; i < DATA_WIDTH; i++){ 
    if (one[i] != two[i]){
      return(0);
      flag = 1; 
    } 
  }
  if (flag != 1){
    return(1);
  }
}

//Function that sets LED colors for specified board by checking the board status array
//and setting the colors based on the pieces key at the start, this is different for 
//if your board is the white board or black board since the lights under your pieces 
//will be dimmer than the representation for the other players pieces:
void setBoard (int board){
  if (board == 1){ //black board
    for (int i = 0; i < DATA_WIDTH; i++){
      if (boardStatus[i] == 9){
        blackStrip.setPixelColor(i, blackStrip.Color(255, 255, 255));
      } else if ((boardStatus[i] == 1) || (boardStatus[i] == 2)){
        blackStrip.setPixelColor(i, blackStrip.Color(255, 0, 0));
      } else if ((boardStatus[i] == 3) || (boardStatus[i] == 4)){
        blackStrip.setPixelColor(i, blackStrip.Color(0, 255, 0));
      } else if ((boardStatus[i] == 5) || (boardStatus[i] == 6)){
        blackStrip.setPixelColor(i, blackStrip.Color(0, 0, 255));
      } else if (boardStatus[i] == 7){
        blackStrip.setPixelColor(i, blackStrip.Color(255, 0, 255));
      } else if (boardStatus[i] == 8){
        blackStrip.setPixelColor(i, blackStrip.Color(245, 200, 0));
      } else if (boardStatus[i] == 0){
        blackStrip.setPixelColor(i, blackStrip.Color(0, 0, 0));
      } else if (boardStatus[i] == 10){
          blackStrip.setPixelColor(i, blackStrip.Color(75, 75, 75));
        } else if ((boardStatus[i] == 11) || (boardStatus[i] == 12)){
          blackStrip.setPixelColor(i, blackStrip.Color(75, 0, 0));
        } else if ((boardStatus[i] == 13) || (boardStatus[i] == 14)){
          blackStrip.setPixelColor(i, blackStrip.Color(0, 75, 0));
        } else if ((boardStatus[i] == 15) || (boardStatus[i] == 16)){
          blackStrip.setPixelColor(i, blackStrip.Color(0, 0, 75));
        } else if (boardStatus[i] == 17){
          blackStrip.setPixelColor(i, blackStrip.Color(75, 0, 75));
        } else if (boardStatus[i] == 18){
          blackStrip.setPixelColor(i, blackStrip.Color(140, 100, 0));
        } 
    }
    blackStrip.show();
  }
    if (board == 0){ //white board
      for (int i = 0; i < DATA_WIDTH; i++){
        if (boardStatus[i] == 10){
          whiteStrip.setPixelColor(i, whiteStrip.Color(255, 255, 255));
        } else if ((boardStatus[i] == 11) || (boardStatus[i] == 12)){
          whiteStrip.setPixelColor(i, whiteStrip.Color(255, 0, 0));
        } else if ((boardStatus[i] == 13) || (boardStatus[i] == 14)){
          whiteStrip.setPixelColor(i, whiteStrip.Color(0, 255, 0));
        } else if ((boardStatus[i] == 15) || (boardStatus[i] == 16)){
          whiteStrip.setPixelColor(i, whiteStrip.Color(0, 0, 255));
        } else if (boardStatus[i] == 17){
          whiteStrip.setPixelColor(i, whiteStrip.Color(255, 0, 255));
        } else if (boardStatus[i] == 18){
          whiteStrip.setPixelColor(i, whiteStrip.Color(245, 200, 0));
        } else if (boardStatus[i] == 0){
          whiteStrip.setPixelColor(i, whiteStrip.Color(0, 0, 0));
        } else if (boardStatus[i] == 9){
            whiteStrip.setPixelColor(i, whiteStrip.Color(75, 75, 75));
          } else if ((boardStatus[i] == 1) || (boardStatus[i] == 2)){
            whiteStrip.setPixelColor(i, whiteStrip.Color(75, 0, 0));
          } else if ((boardStatus[i] == 3) || (boardStatus[i] == 4)){
            whiteStrip.setPixelColor(i, whiteStrip.Color(0, 75, 0));
          } else if ((boardStatus[i] == 5) || (boardStatus[i] == 6)){
            whiteStrip.setPixelColor(i, whiteStrip.Color(0, 0, 75));
          } else if (boardStatus[i] == 7){
            whiteStrip.setPixelColor(i, whiteStrip.Color(75, 0, 75));
          } else if (boardStatus[i] == 8){
            whiteStrip.setPixelColor(i, whiteStrip.Color(140, 100, 0));
          } 
      }
      whiteStrip.show();
    }
}


//Function that attempts to connect to WiFi:
void connectWiFi() {
  Serial.println();
  Serial.println("Connecting to: " + String(WIFI_SSID));
  
  // Set WiFi mode to station (client)
  WiFi.mode(WIFI_STA);
  
  // Initiate connection with SSID and PSK
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  
  // Blink LED while we wait for WiFi connection
  while ( WiFi.status() != WL_CONNECTED ) {
    Serial.write("Wifi not connected");
    delay(100);
  }
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


//Function that trys to post to phant both the last_move and turn
//it will return a number, 1 if it is successful, and 0 if it is not:
int postToPhant()
{
  // Declare an object from the Phant library - phant
  Phant phant(http_site, PublicKey, PrivateKey);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String postedID = "ThingDev-" + macID;

//*******************add data here
//*******************format: http:   //data.sparkfun.com/input/[publicKey]?private_key=[privateKey]&last_move=[value]&turn=[value]
  // Add the four field/value pairs defined by our stream:
  phant.add("last_move", postWords);
  phant.add("turn", postTurn);

  if (!client.connect(http_site, http_port)) 
  {
    // If we fail to connect, return 0.
    return 0;
  }
  // If we successfully connected, print our Phant post:
  client.print(phant.post());
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line); 
  }

  return 1; // Return success
}


//Function that performs an HTTP GET request to a remote page (data.sparkfun):
int getPage() {
  
  // Attempt to make a connection to the remote server
  if ( !client.connect(http_site, http_port) ) {
    return 0;
  }
  
  // Make an HTTP GET request
    client.print("GET /output/");
    client.print(PublicKey);
    client.print("/latest");
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(http_site);
    /* if you close the connection you cannot use it anymore, but
       if you were using an HTTP 1.0 GET you would have to since it
       can't maintain an open connection:
    */
    //client.println("Connection: close");
  
    client.println();
  
  return 1;
}

//Function that takes in which board you have, what line of the LCD
//to print on, and the things you want to print and writes them
//to that line of the LCD:
void writeToScreen(int board, int line, String Str1){
  //Function that writes to the serial LCD screen
  Serial.println(Str1); //for debugging purposes
  if (board == 0){  //white board
    if (line == 1){
      whiteSerial.write(254);
      whiteSerial.write(128);
    } else if (line == 2){
      whiteSerial.write(254);
      whiteSerial.write(192);
    }
    whiteSerial.print(Str1);
  } else if (board == 1){  //black board
    if (line == 1){
      blackSerial.write(254);
      blackSerial.write(128);
    } else if (line == 2){
      whiteSerial.write(254);
      whiteSerial.write(192);
    }
    blackSerial.print(Str1);
  } 
}

//Function that reads all of the inputs from the shift registers and puts the 
//first 64 into the magnet input array, and the last 8 into the button states array:
void read_shift_regs(int board)
{
    long bitVal;
    long bytesVal[DATA_WIDTH];

    // Trigger a parallel Load to latch the state of the data lines 
    // of the shift register:
    digitalWrite(clockEnablePin, HIGH);
    digitalWrite(ploadPin, LOW);
    delayMicroseconds(PULSE_WIDTH_USEC);
    digitalWrite(ploadPin, HIGH);
    digitalWrite(clockEnablePin, LOW);

    // Loop to read each bit value from the serial out line
    // of the SN74HC165N:
    for(int i = 0; i < DATA_WIDTH; i++)
    {
        bitVal = digitalRead(dataPin);

        bytesVal[i] = bitVal;

        //Pulse the shift register Clock (rising edge shifts the next bit):
        digitalWrite(clockPin, HIGH);
        delayMicroseconds(PULSE_WIDTH_USEC);
        digitalWrite(clockPin, LOW);
    }
    for (int i = 0; i < 64; i++){
      if (board == 0){ //white
        whiteMagnetInput[i] = bytesVal[i];
      } else if (board == 1){ //black
        blackMagnetInput[i] = bytesVal[i];
      }
    }
    
    for (int i = 64; i < DATA_WIDTH; i++){
      //Buttons hooked up (pulled high when not pushed): enter=0, draw=1, concede=2, castle=3, castleSide=4, 5,6&7 are pulled high
      buttonStates[i-64] = bytesVal[i];
    }

}


//Function that deals with castling on either board, either side, and prints
//instuctions on the LCD screen:
void castling(int board){ //castle button was pushed
  //Buttons hooked up (pulled high when not pushed): enter=0, draw=1, concede=2, castle=3, castleSide=4, 5,6&7 are pulled high
  writeToScreen(board, 1, "Want to casle?  ");
  writeToScreen(board, 2, "Y-Enter N-Castle");
  read_shift_regs(board);
  while ((buttonStates[0] == 1) && (buttonStates[3] == 1)){
    delay(100);
    read_shift_regs(board);
  }
  if (buttonStates[3] == 0){
    writeToScreen(board, 1, "Castle canceled ");
    writeToScreen(board, 2, "                ");
  } else if (buttonStates[0] == 1){
  writeToScreen(board, 1, "Choose castle   ");
  writeToScreen(board, 2, "side then Enter ");
    while (buttonStates[0] == 1){
      delay(100);
      read_shift_regs(board);
    }
  }
  castleSide = buttonStates[4];
  if (board == 0){
    if (castleSide == 1){ //king side castle
      while ((whiteMagnetInput[4] == 0) || (whiteMagnetInput[7] == 0)){
        writeToScreen(board, 1, "Castle king side");
        writeToScreen(board, 2, "take pieces off ");
        whiteStrip.setPixelColor(4, whiteStrip.Color(255, 255, 255));
        whiteStrip.setPixelColor(7, whiteStrip.Color(255, 255, 255));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(4, whiteStrip.Color(0, 0, 0));
        whiteStrip.setPixelColor(7, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      while ((whiteMagnetInput[4] == 1) || (whiteMagnetInput[7] == 1)){
        writeToScreen(board, 1, "Castle king side");
        writeToScreen(board, 2, "put pieces on   ");
        whiteStrip.setPixelColor(4, whiteStrip.Color(255, 0, 0));
        whiteStrip.setPixelColor(7, whiteStrip.Color(245, 200, 0));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(4, whiteStrip.Color(0, 0, 0));
        whiteStrip.setPixelColor(7, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      boardStatus[4] = 2;
      boardStatus[7] = 8;
      postWords = "~Last turn: 0-0 ";
      postTurn = "Black's Turn";
      postToPhant();
      turn = 1;
      setBoard(0);
    } else if (castleSide == 0){
      while ((whiteMagnetInput[4] == 0) || (whiteMagnetInput[0] == 0)){
        writeToScreen(board, 1, "Queen side      ");
        writeToScreen(board, 2, "take pieces off ");
        whiteStrip.setPixelColor(4, whiteStrip.Color(255, 255, 255));
        whiteStrip.setPixelColor(0, whiteStrip.Color(255, 255, 255));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(4, whiteStrip.Color(0, 0, 0));
        whiteStrip.setPixelColor(0, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      while ((whiteMagnetInput[4] == 1) || (whiteMagnetInput[0] == 1)){
        writeToScreen(board, 1, "Queen side      ");
        writeToScreen(board, 2, "put pieces on   ");
        whiteStrip.setPixelColor(4, whiteStrip.Color(255, 0, 0));
        whiteStrip.setPixelColor(0, whiteStrip.Color(245, 200, 0));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(4, whiteStrip.Color(0, 0, 0));
        whiteStrip.setPixelColor(0, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      boardStatus[4] = 1;
      boardStatus[0] = 8; 
      postWords = "~Last turn:0-0-0";
      postTurn = "Black's Turn";
      postToPhant();
      turn = 1;
      setBoard(0); 
    }
  } else if (board == 1){
     if (castleSide == 1){ //king side castle
      while ((blackMagnetInput[60] == 0) || (blackMagnetInput[63] == 0)){
        writeToScreen(board, 1, "Castle king side");
        writeToScreen(board, 2, "take pieces off ");
        blackStrip.setPixelColor(60, blackStrip.Color(255, 255, 255));
        blackStrip.setPixelColor(63, blackStrip.Color(255, 255, 255));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(60, blackStrip.Color(0, 0, 0));
        blackStrip.setPixelColor(63, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      while ((whiteMagnetInput[60] == 1) || (whiteMagnetInput[63] == 1)){
        writeToScreen(board, 1, "Castle king side");
        writeToScreen(board, 2, "put pieces on   ");
        blackStrip.setPixelColor(60, blackStrip.Color(255, 0, 0));
        blackStrip.setPixelColor(63, blackStrip.Color(245, 200, 0));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(60, blackStrip.Color(0, 0, 0));
        blackStrip.setPixelColor(63, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      boardStatus[60] = 12;
      boardStatus[63] = 18;
      postWords = "~Last turn: 0-0 ";
      postTurn = "White's Turn";
      postToPhant();
      turn = 0;
      setBoard(1);  
    } else if (castleSide == 0){
      while ((blackMagnetInput[60] == 0) || (blackMagnetInput[56] == 0)){
        writeToScreen(board, 1, "Queen side      ");
        writeToScreen(board, 2, "take pieces off ");
        blackStrip.setPixelColor(60, blackStrip.Color(255, 255, 255));
        blackStrip.setPixelColor(56, blackStrip.Color(255, 255, 255));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(60, blackStrip.Color(0, 0, 0));
        blackStrip.setPixelColor(56, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      while ((blackMagnetInput[60] == 1) || (blackMagnetInput[56] == 1)){
        writeToScreen(board, 1, "Queen side      ");
        writeToScreen(board, 2, "put pieces on   ");
        blackStrip.setPixelColor(60, blackStrip.Color(255, 0, 0));
        blackStrip.setPixelColor(56, blackStrip.Color(245, 200, 0));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(60, blackStrip.Color(0, 0, 0));
        blackStrip.setPixelColor(56, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
        read_shift_regs(board);
      }
      boardStatus[60] = 11;
      boardStatus[56] = 18;  
      postWords = "~Last turn:0-0-0";
      postTurn = "White's Turn";
      postToPhant();
      turn = 0;
      setBoard(1);  
    }
  }
  read_shift_regs(board);
  //if enter button pushed then make the changes
}


//Function that deals with putting out a draw request:
void draw(int board){ //draw button was pushed
  //Buttons hooked up (pulled high when not pushed): enter=0, draw=1, concede=2, castle=3, castleSide=4, 5,6&7 are pulled high
  writeToScreen(board, 1, "Are you sure?   ");
  writeToScreen(board, 2, "Y-Enter N-Draw  ");
  delay(100);
  read_shift_regs(board);
  while((buttonStates[0] == 1) && (buttonStates[1] == 1)){
    delay(500);
    read_shift_regs(board);
  }
  
  if (board == 0){ //white board
    if (buttonStates[0] == 0){
      writeToScreen(0, 1, "Draw game sent. ");
      writeToScreen(0, 2, "Wait for black  ");
      postWords = "~Draw proposed   ";
      postTurn = "Black's Turn";
      postToPhant();
      turn = 1;
      setBoard(0);
      drawRequest = 1;
    } else if (buttonStates[1] == 0) {
      writeToScreen(0, 1, "Draw Cancled    ");
      writeToScreen(0, 1, "                ");
      drawRequest = 0;
      delay(1000);
    }
  } else if (board == 1) { //black board
    if (buttonStates[0] == 0){
      writeToScreen(1, 1, "Draw game sent. ");
      writeToScreen(1, 2, "Wait for white  ");
      postWords = "~Draw proposed   ";
      postTurn = "White's Turn";
      postToPhant();
      turn = 0;
      setBoard(1);
      drawRequest = 1;
    } else if (buttonStates[0] == 0) {
      writeToScreen(1, 1, "Draw Cancled    ");
      writeToScreen(1, 1, "                ");
      drawRequest = 0;
      delay(1000);
    }
    }
}


//Fuction that deals with conceding to the the other player:
void concede(int board){ //concede button was pushed
  //Buttons hooked up (pulled high when not pushed): enter=0, draw=1, concede=2, castle=3, castleSide=4, 5,6&7 are pulled high
  writeToScreen(board, 1, "Are you sure?   ");
  writeToScreen(board, 2, "Y-Enter N-Conc  ");
  read_shift_regs(board);
  while ((buttonStates[0] == 1) && (buttonStates[2] == 1)){
    delay(100);
    read_shift_regs(board);
  }
  if (board == 0){ //white board
    if (buttonStates[0] == 0){
      writeToScreen(0, 1, "Concede sent    ");
      writeToScreen(0, 2, "Black wins game.");
      postWords = "~White concedes  ";
      postTurn = "Black's Turn";
      postToPhant();
      turn = 1;
      setBoard(0);
    } else if (buttonStates[2] == 0) {
      writeToScreen(0, 1, "Concede canceled");
      writeToScreen(0, 2, "                ");
      delay(1000);
    }
  } else if (board == 1) {
    if (buttonStates[0] == 0){
      writeToScreen(1, 1, "Concede sent    ");
      writeToScreen(1, 2, "White wins game.");
      postWords = "~Black concedes  ";
      postTurn = "White's Turn";
      postToPhant();
      turn = 0;
      setBoard(1);
    } else if (buttonStates[2] == 0) {
      writeToScreen(1, 1, "Concede canceled");
      writeToScreen(1, 2, "                ");
      delay(1000);
    }
  }
}


//Fuction that checks if an en passant move was made by a pawn
//and sets a flag if it is:
void enPassant(){
  //check if pawn moved went diagonally
  //check if there was a piece where it moved
  //if not 'capture' the piece right behind the pawn
  if ((moveFromNumber % 8) != (moveToNumber % 8)){ //if the pawn moved diagonally
    if (captureCheck == 0){ //nothing was where the pawn moved to
        enPassantCheck = 1;
    }
  }
}


//Fuction that checks if a pawn has reached the other end of the board
//and if it has, promotes it to a Queen, flashing the LED on the board
//and displaying on the LCD for the player to do so:
void pawnPromotion(int board){
  //check pawn moved is at the other end of the board
  if (board == 0){
    for (int i = 56; i < 64; i++){
      if (boardStatus[i] == 9){
        //pawnPromotion!
        pawnPromotionCheck = 1;
        pawnToQueen = i;
      } 
    }

    if (pawnPromotionCheck == 1){
      while (whiteMagnetInput[pawnToQueen] == 0){
        writeToScreen(0, 2, "Take Pawn off   ");
        whiteStrip.setPixelColor(moveToNumber, whiteStrip.Color(255, 255, 255));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(moveToNumber, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
      }
      while (whiteMagnetInput[pawnToQueen] == 1){
        writeToScreen(0, 2, "Put Queen on    ");
        whiteStrip.setPixelColor(moveToNumber, whiteStrip.Color(150, 0, 150));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(moveToNumber, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
      }
      pawnPromotionCheck = 0;
      pawnPromotionMade = 1;
      boardStatus[pawnToQueen] = 7;
    }
  } else if(board == 1){
    for (int i = 0; i < 8; i++){
      if (boardStatus[i] == 10){
        pawnPromotionCheck = 1;
        pawnToQueen = i;
      }
    }

    if (pawnPromotionCheck == 1){
      while (blackMagnetInput[pawnToQueen] == 0){
        writeToScreen(1, 2, "Take Pawn off   ");
        blackStrip.setPixelColor(moveToNumber, blackStrip.Color(255, 255, 255));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(moveToNumber, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
      }
      while (blackMagnetInput[pawnToQueen] == 1){
        writeToScreen(1, 2, "Put Queen on    ");
        blackStrip.setPixelColor(moveToNumber, blackStrip.Color(255, 0, 255));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(moveToNumber, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
      }
      pawnPromotionCheck = 0;
      pawnPromotionMade = 1;
      boardStatus[pawnToQueen] = 17;
    }
  } 
  
}

//Function that looks at the shift register inputs for the buttons
//and checks if any of them were pushed (pulled low):
void checkButtons(){
    if (buttonStates[1] == 0){ //draw button pushed
      draw(physicalBoard);
    }
    if (buttonStates[2] == 0){ //concede button pushed
      concede(physicalBoard);
    }
    if (buttonStates[3] == 0){ //caslte button pushed
      castling(physicalBoard);
    }
}


//This is one of the main functions, called when the magnets switch inputs
//on the board have changed (meaning something was moved) it looks through every 
//space, to see which piece was moved, looks up the code of that piece, and then waits
//for you to put it back down. If you pick up more than one piece, it tells to to put 
//them all back until the board is the way it was. If you do move a piece and put it 
//back down, it updates the board status array, updates the variables that keep track 
//of where the piece moved from, where it moved to, what the piece was, if there was a 
//capture, and that the move was completed:
void checkPiece(int board){
  //Check the difference in the magnet boards and find out what piece moved: 
  if (board == 0){  //white
    for (int i = 0; i < DATA_WIDTH; i++){
      if ((arraysEqual(whiteMagnetInput, oldWhiteMagnetInput)) == 0){
        if ((whiteMagnetInput[i] == 1) && (oldWhiteMagnetInput[i] == 0) && (boardChange == 0)){
          pieceCodeMoved = boardStatus[i];
          Serial.write("Piece moved: ");
          Serial.println(pieceCodeMoved); //debugging
          if ((pieceCodeMoved == 1) || (pieceCodeMoved == 2)){
            pieceMoved = 'R';
            pawnMoved = 0;
          } else if ((pieceCodeMoved == 3) || (pieceCodeMoved == 4)){
            pieceMoved = 'N';
            pawnMoved = 0;
          } else if ((pieceCodeMoved == 5) || (pieceCodeMoved == 6)){
            pieceMoved = 'B';
            pawnMoved = 0;
          } else if (pieceCodeMoved == 7){
            pieceMoved = 'Q';
            pawnMoved = 0;
          } else if (pieceCodeMoved == 8){
            pieceMoved = 'K';
            pawnMoved = 0;
          }else if (pieceCodeMoved == 9){
            pieceMoved = ' ';
            pawnMoved = 1;
          }
          if (pieceCodeMoved != 0){
            positionMovedFrom = i;
            firstPieceMoved = i;
            Serial.write("Position moved from: ");
            Serial.println(positionMovedFrom);
            //boardStatus[i] = 0;
            boardChange = 1;
            moveFromNumber = i;
          } else {
            noChangeFlag = 1;
          }
        } else if ((whiteMagnetInput[i] == 1) && (oldWhiteMagnetInput[i] == 0) && (boardChange == 1) && (firstPieceMoved != i)){
          while((arraysEqual(whiteMagnetInput, oldWhiteMagnetInput)) == 0){
            Serial.write("Error, too many pieces picked up, put them back.");
            Serial.println( );
            writeToScreen(0, 2, "Put pieces back ");
            delay(1000);
            read_shift_regs(0);
            Serial.write("Magnet input ");
            for (int i=0; i<DATA_WIDTH; i++){
              Serial.print(whiteMagnetInput[i]);
            }
            Serial.println();
          }
          boardChange = 0;
          noChangeFlag = 1;
        } else {
         noChangeFlag = 1;
        }
      }
    }
  for (int i = 0; i < DATA_WIDTH; i++){
      if ((arraysEqual(whiteMagnetInput, oldWhiteMagnetInput)) == 0){
        if((whiteMagnetInput[i] == 0) && (oldWhiteMagnetInput[i] == 1) && (boardChange == 1)){
            delay(pieceDelay);
            read_shift_regs('white');
            //Check if after the delay the piece is still there:
            if(whiteMagnetInput[i] == 0){
                  //check if anything got captured:
                  if (boardStatus[i] != 0){
                    captureCheck = 1;
                  } else if (boardStatus[i] == 0){
                    captureCheck = 0;
                  }
                  boardStatus[i] = pieceCodeMoved;
                  Serial.write("Position moved to: ");
                  Serial.println(i);
                  Serial.write("Position moved from: ");
                  Serial.println(positionMovedFrom);
                  boardStatus[positionMovedFrom] = 0;
                  moveToNumber = i;
                  boardChange = 0;
                  noChangeFlag = 0;
                  if (pawnMoved == 1){
                      enPassant();
                      pawnPromotion(1);
                  }
            } else {
                noChangeFlag = 1;
            }
        }
      } else {
          noChangeFlag = 1;
      }
    }
    delay(500);
  }
  
  if (board == 1){  //black 
      for (int i = 0; i < DATA_WIDTH; i++){
      if ((arraysEqual(blackMagnetInput, oldBlackMagnetInput)) == 0){
        if ((blackMagnetInput[i] == 1) && (oldBlackMagnetInput[i] == 0) && (boardChange == 0)){
          pieceCodeMoved = boardStatus[i];
          Serial.write("Piece moved: ");
          Serial.println(pieceCodeMoved); //debugging
          if ((pieceCodeMoved == 11) || (pieceCodeMoved == 12)){
            pieceMoved = 'R';
            pawnMoved = 0;
          } else if ((pieceCodeMoved == 13) || (pieceCodeMoved == 14)){
            pieceMoved = 'N';
            pawnMoved = 0;
          } else if ((pieceCodeMoved == 15) || (pieceCodeMoved == 16)){
            pieceMoved = 'B';
            pawnMoved = 0;
          } else if (pieceCodeMoved == 17){
            pieceMoved = 'Q';
            pawnMoved = 0;
          } else if (pieceCodeMoved == 18){
            pieceMoved = 'K';
            pawnMoved = 0;
          }else if (pieceCodeMoved == 10){
            pieceMoved = ' ';
            pawnMoved = 1;
          }
          if (pieceCodeMoved != 0){
            positionMovedFrom = i;
            firstPieceMoved = i;
            Serial.write("Position moved from: ");
            Serial.println(positionMovedFrom);
            //boardStatus[i] = 0;
            boardChange = 1;
            moveFromNumber = i;
          } else {
            noChangeFlag = 1;
          }
        } else if ((blackMagnetInput[i] == 1) && (oldBlackMagnetInput[i] == 0) && (boardChange == 1) && (firstPieceMoved != i)){
          while((arraysEqual(blackMagnetInput, oldBlackMagnetInput)) == 0){
            Serial.write("Error, too many pieces picked up, put them back.");
            Serial.println( );
            writeToScreen(0, 2, "Put pieces back ");
            delay(1000);
            read_shift_regs(1);
            Serial.write("Magnet input "); 
            for (int i=0; i<DATA_WIDTH; i++){
              Serial.print(blackMagnetInput[i]);
            }
            Serial.println();
          }
          boardChange = 0;
          noChangeFlag = 1;
        } else {
         noChangeFlag = 1;
        }
      }
    }
  for (int i = 0; i < DATA_WIDTH; i++){
      if ((arraysEqual(blackMagnetInput, oldBlackMagnetInput)) == 0){
        if((blackMagnetInput[i] == 0) && (oldBlackMagnetInput[i] == 1) && (boardChange == 1)){
            delay(pieceDelay);
            read_shift_regs(1);
            //Check if after the delay the piece is still there:
            if(blackMagnetInput[i] == 0){
                  //check if anything got captured:
                  if (boardStatus[i] != 0){
                    captureCheck = 1;
                  } else if (boardStatus[i] == 0){
                    captureCheck = 0;
                  }
                  boardStatus[i] = pieceCodeMoved;
                  Serial.write("Position moved to: ");
                  Serial.println(i);
                  Serial.write("Position moved from: ");
                  Serial.println(positionMovedFrom);
                  boardStatus[positionMovedFrom] = 0;
                  moveToNumber = i;
                  boardChange = 0;
                  noChangeFlag = 0;
                  if (pawnMoved == 1){
                      enPassant();
                      pawnPromotion(1);
                  }
            } else {
                noChangeFlag = 1;
            }
        }
      } else {
          noChangeFlag = 1;
      }
    }
    delay(500);
  }

}


//Fuction that takes a number (0-63) and assigns it its actual 
//name based on the chess naming convention (coloms a-h, rows 1-8)
//and puts that info into a variable, either moveFrom or moveTo 
//based on what was put into the fuction (in the int Move, 0 means
//where it moved from, 1 means where it moved to):
void findPositionName (int Move, int Place){
  //////FIX FOR BLACK BOARD!!!!!!!!!
  col = (Place % 8) + 97;
  row = (Place / 8) + 1;
  if (Move == 0){ //Checking where the piece moved from
    moveFrom = String(String(col) + row);
  } else if (Move == 1){
    moveTo = String(String(col) + row);
  }
  
}


//Fuction only used at the start of the game, sent
//by the white board, posting to phant
//stating that it is a new game
//and White's Turn (white always goes first in Chess):
int firstPost(){
  // Declare an object from the Phant library - phant
  Phant phant(http_site, PublicKey, PrivateKey);

  // Do a little work to get a unique-ish name. Append the
  // last two bytes of the MAC (HEX'd) to "Thing-":
  uint8_t mac[WL_MAC_ADDR_LENGTH];
  WiFi.macAddress(mac);
  String macID = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) +
                 String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
  macID.toUpperCase();
  String postedID = "ThingDev-" + macID;

//*******************add data here
//*******************format: http:   //data.sparkfun.com/input/[publicKey]?private_key=[privateKey]&last_move=[value]&turn=[value]
  // Add the four field/value pairs defined by our stream:
  phant.add("last_move", "~New Game!       ");
  phant.add("turn", "White's Turn");

  if (!client.connect(http_site, http_port)) 
  {
    // If we fail to connect, return 0.
    return 0;
  }
  // If we successfully connected, print our Phant post:
  client.print(phant.post());
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line); 
  }

  return 1; // Return success
}


//Fuction that takes the info from Phant and 
//and any other fuctions, to see what the last
//move was, and if there was a capture, and 
//update the board status array
//it will also blink the tile if a piece 
//was captured until it is taken off
//before updating the LEDs to the last
//opponents move:
void updateNewPosition(int board){
  if (enPassantCheck == 1){
    moveFromNumber = (((((int)getPosition.charAt(1)) - 96) + ((((int)getPosition.charAt(2)) - 49) * 8)) - 1);
    pieceCodeMoved = boardStatus[moveFromNumber];
    moveToNumber = (((((int)getPosition.charAt(3)) - 96) + ((((int)getPosition.charAt(4)) - 49) * 8)) - 1);
    boardStatus[moveFromNumber] = 0;
    boardStatus[moveToNumber] = pieceCodeMoved;
    if (board == 0){
      read_shift_regs(0); //white
      int takenNumber = moveToNumber - 8;
      while(whiteMagnetInput[takenNumber] == 0){
        Serial.write("Take captured piece off");
        Serial.println();
        whiteStrip.setPixelColor(takenNumber, whiteStrip.Color(255, 255, 255));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(takenNumber, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
        read_shift_regs(0);
      }
      if (whiteMagnetInput[takenNumber] == 1){
        oldWhiteMagnetInput[takenNumber] = whiteMagnetInput[takenNumber];
        removedPiece = 1; 
      }
      read_shift_regs(0);
    } else if (board == 1){
      read_shift_regs(1); //black
      int takenNumber = moveToNumber + 8;
      while(blackMagnetInput[takenNumber] == 0){
      Serial.write("Take captured piece off");
        Serial.println();
        blackStrip.setPixelColor(takenNumber, blackStrip.Color(255, 255, 255));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(takenNumber, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
        read_shift_regs(1);
      }
      if (blackMagnetInput[moveToNumber] == 1){
        oldBlackMagnetInput[moveToNumber] = blackMagnetInput[moveToNumber];
        removedPiece = 1; 
      } 
      read_shift_regs(1);
    }
    
    enPassantCheck = 0;
  } else if (yourPieceCaptured == 0){ //if nothing was captured
    moveFromNumber = (((((int)getPosition.charAt(1)) - 96) + ((((int)getPosition.charAt(2)) - 49) * 8)) - 1);
    pieceCodeMoved = boardStatus[moveFromNumber];
    moveToNumber = (((((int)getPosition.charAt(3)) - 96) + ((((int)getPosition.charAt(4)) - 49) * 8)) - 1);
    boardStatus[moveFromNumber] = 0;
    if (pawnPromotionMade == 1){
      boardStatus[moveToNumber] = 'Q';
      pawnPromotionMade = 0;
    } else if (pawnPromotionMade == 0){
      boardStatus[moveToNumber] = pieceCodeMoved;
    }
  } else if (yourPieceCaptured == 1){ //something was captured
    moveFromNumber = (((((int)getPosition.charAt(1)) - 96) + ((((int)getPosition.charAt(2)) - 49) * 8)) - 1);
    pieceCodeMoved = boardStatus[moveFromNumber];
    moveToNumber = (((((int)getPosition.charAt(4)) - 96) + ((((int)getPosition.charAt(5)) - 49) * 8)) - 1);
    boardStatus[moveFromNumber] = 0;
    if (pawnPromotionMade == 1){
      boardStatus[moveToNumber] = 'Q';
      pawnPromotionMade = 0;
    } else if (pawnPromotionMade == 0){
      boardStatus[moveToNumber] = pieceCodeMoved;
    }
  
  
  if(board == 0){ //white board
      read_shift_regs(0); //white
      while(whiteMagnetInput[moveToNumber] == 0){
        Serial.write("Take captured piece off");
        Serial.println();
        whiteStrip.setPixelColor(moveToNumber, whiteStrip.Color(255, 255, 255));
        whiteStrip.show();
        delay(1000);
        whiteStrip.setPixelColor(moveToNumber, whiteStrip.Color(0, 0, 0));
        whiteStrip.show();
        delay(1000);
        read_shift_regs(0);
        yourPieceCaptured = 0;
      }
      if (whiteMagnetInput[moveToNumber] == 1){
        oldWhiteMagnetInput[moveToNumber] = whiteMagnetInput[moveToNumber];
        removedPiece = 1; 
      } 
      read_shift_regs(0);
  } else if (board == 1){ //black board
      read_shift_regs(1); //black
      while(blackMagnetInput[moveToNumber] == 0){ //blink captured spot until piece is taken off
        Serial.write("Take captured piece off");
        Serial.println();
        blackStrip.setPixelColor(moveToNumber, blackStrip.Color(255, 255, 255));
        blackStrip.show();
        delay(1000);
        blackStrip.setPixelColor(moveToNumber, blackStrip.Color(0, 0, 0));
        blackStrip.show();
        delay(1000);
        read_shift_regs(1);
        yourPieceCaptured = 0;
      }

      if (blackMagnetInput[moveToNumber] == 1){
        oldBlackMagnetInput[moveToNumber] = blackMagnetInput[moveToNumber];
        removedPiece = 1; 
      } 
      read_shift_regs(1);
    }
      
   
  }
  updatedLights = 1;
  yourPieceCaptured = 0;
}

//This is one of the required functions in Arduino that runs once 
//when the board starts up, so it is the place to begin all
//the screens and initialize all the pins to their modes.
//In this function it also checks the board side switch to to determine 
//which board it is, and if it is the white board send out the first 
//post. It also sets up the communication and tries to pull data 
//from Phant:
void setup(){
    Serial.begin(9600); //for debugging
    connectWiFi(); // Connect to WiFi
    // Initialize our digital pins:
    pinMode(ploadPin, OUTPUT);
    pinMode(clockEnablePin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, INPUT);
    pinMode(PIXEL_PIN, OUTPUT);
    pinMode(boardSidePin, INPUT);

    digitalWrite(clockPin, LOW);
    digitalWrite(ploadPin, HIGH);

    side = digitalRead(boardSidePin);

    if (side == 0){ //switch is on the black side
      physicalBoard = 1; 
    } else if (side == 1){ //switch is on the white side
      physicalBoard = 0;
      whiteSerial.begin(9600);
      delay(500);
      if (firstPost() == 0){
        Serial.write("First post failed");
      }
    }

    delay(500);
    if (getPage() == 0){
      Serial.println("Get page failed");
    }
}


//This is the other function required by Arduino, it runs and loops after 
//the setup function runs once. This is where everything is called from.
//First it checks if any info is coming in from Phant, and if it is puts 
//the important info into a variable readString. It then gets the last turn 
//info and puts that into the variable displayWords. From there it checks 
//which board it is, and runs all the checks based on whos turn it is.
//If it is the boards turn, it waits for a move to be made and when it 
//is (sensed by a change in the reed switchs from the shift registers) it
//calls the checkPiece Function. If after that a move has been made, it updates 
//the LCD and sends that info to Phant, and changes that turn.
//If it isn't the boards turn, it waits, reading info from Phant, until it is
//the boards turn. If the connection with Phant is interupted or lost, it 
//writes that to the LCD and trys to connect again.
void loop(){
  if ( client.available() ) {
    char c = client.read();
    if (c == '~'){
      startReading = 1;
    }
    if (startReading == 1){
        if (readString.length() < 30){
          readString += c;
          if (c == '~'){
            displayWords = "";
          } else if (displayWords.length() < 16){
            displayWords += c;
          }
        }
        if (readString.length() == 30){
          Serial.println();
          Serial.println(readString);
          startReading = 0;
        }
    }  
  }
  
  if (physicalBoard == 0){ //white board
    read_shift_regs(0); //white
    checkButtons();
    //Update the LED on the board:
    setBoard(0); //white

    if (readString.endsWith("Black's Turn")){
      turn = 1;
      Serial.println("Waiting for black to take turn");
      Serial.println(displayWords);
      readString = "";
      setBoard(0);
      removedPiece = 0;
      updatedLights = 0; 
      enPassantCheck = 0;
      writeToScreen(0, 1, displayWords);       //Write to white screen, first line, the last turn (from black)
      writeToScreen(0, 2, "Black's Turn    "); //Write to white screen, second line, "Black's Turn"
      if (getPage() == 0){
        Serial.println("Get page failed");
      }
      delay(5000);
    } else if (readString.endsWith("White's Turn")){
      turn = 0;
      Serial.println(displayWords);

      if (displayWords.indexOf('Black concedes') != -1){
        writeToScreen(0, 1, displayWords);
        writeToScreen(0, 2, "White wins!     ");
        //winSequence(0);
        updatedLights = 1;
      } else if (displayWords.indexOf('Draw proposed') != -1){
        writeToScreen(0, 1, displayWords);
        writeToScreen(0, 2, "Y-Draw N-Enter  ");
        read_shift_regs(physicalBoard);
        while((buttonStates[0] == 1) && (buttonStates[1] == 1)){
          delay(500);
          read_shift_regs(physicalBoard);
        }
        if (buttonStates[0] == 0){
          writeToScreen(0, 1, "Draw denied     ");
          writeToScreen(0, 2, "sending decision");
          postWords = "~Draw denied     ";
          postTurn = "Black's Turn";
          postToPhant();
          turn = 1;
          setBoard(0);
        } else if (buttonStates[1] == 0){
          writeToScreen(0, 1, "Draw accepted   ");
          writeToScreen(0, 2, "sending decision");
          postWords = "~Draw accepted   ";
          postTurn = "Black's Turn";
          postToPhant();
          turn = 1;
          setBoard(0);
        }
        updatedLights = 1;
      } else if (displayWords.indexOf('Draw denied') != -1){
        writeToScreen(0, 1, "Draw denied     ");
        writeToScreen(0, 2, "White's Turn    ");
        drawRequest = 0;
        updatedLights = 1;
      } else if (displayWords.indexOf('Draw accepted') != -1){
        writeToScreen(0, 1, "Draw accepted   ");
        writeToScreen(0, 2, "Game is a draw! ");
        //drawSquence(0);
        updatedLights = 1;
      } else if (displayWords.indexOf('New Game!') != -1){
        //It is the first turn, don't need position info
        firstTurn = 1;
      } else if (displayWords.indexOf('0-0-0') != -1){
        boardStatus[60] = 11;
        boardStatus[56] = 18;
        setBoard(0);
        writeToScreen(0, 1, displayWords);
        writeToScreen(0, 2, "White's Turn    ");
        updatedLights = 1;
      } else if (displayWords.indexOf('0-0') != -1){
        boardStatus[60] = 12;
        boardStatus[63] = 18;
        setBoard(0);
        writeToScreen(0, 1, displayWords);
        writeToScreen(0, 2, "White's Turn    ");
        updatedLights = 1;
      } else if ((displayWords.indexOf('EP') != -1) && (removedPiece == 0)){ //last move was an en passant
        enPassantCheck = 1;
      } else if (displayWords.indexOf('=Q') != -1){
        pawnPromotionMade = 1;
      } else if (displayWords.indexOf('x') == -1){ //no capture 
        yourPieceCaptured = 0;
        getPosition = displayWords.substring(11);
        Serial.println(getPosition);
      } else if ((displayWords.indexOf('x') != -1) && (removedPiece == 0)){ 
        //Something was captured
        getPosition = displayWords.substring(10);
        Serial.println(getPosition);
        yourPieceCaptured = 1;
      }

      if (firstTurn == 1){ //first turn
        //don't need position info
        firstTurn = 0;
      } else if ((firstTurn == 0) && (updatedLights == 0)){
      //Parse the last move and update the board position array:
        updateNewPosition(0);
      }
      //Update the LED on the board:
      setBoard(0); //white
      writeToScreen(0, 1, displayWords);       //Write to white screen, first line, the last turn (from black)
      writeToScreen(0, 2, "White's Turn    "); //Write to white screen, second line, "White's Turn"
      readString = "";
      Serial.println("Take your turn!");
      setBoard(0);
      read_shift_regs(0); //white
      checkButtons();



      int testArrays = arraysEqual(whiteMagnetInput, oldWhiteMagnetInput);
      Serial.write("Arrays equal? ");
      Serial.println(testArrays);
      Serial.write("Magnet input ");
      for (int i=0; i<DATA_WIDTH; i++){
        Serial.print(whiteMagnetInput[i]);
      }
      Serial.println();
      Serial.write("Position info ");
      for (int i=0; i<DATA_WIDTH; i++){
        Serial.print(boardStatus[i]);
      }
      Serial.println();
      Serial.println(boardChange);
      
    if (testArrays == 0){
        //Allow the player some wait time to be able to change their mind:
        delay(pieceDelay);
        //Read the magnests again:
        read_shift_regs(0); //white
        checkButtons();
        //Check what piece moved from where to where, and if it captured anything:
        checkPiece(0); //white
        //Check if there was a full move made (picked piece up and put it back down):
        if (noChangeFlag == 0){
           //Update the oldWhiteMagnetInput array:
           for (int i = 0; i < DATA_WIDTH; i++){
              oldWhiteMagnetInput[i] = whiteMagnetInput[i];
            }
            //Write that it is black's turn on the LCD screen:
            findPositionName(0, moveFromNumber);
            findPositionName(1, moveToNumber);
            if (captureCheck == 0){
              displayWords = "Last Move: " + String(pieceMoved) + moveFrom + moveTo;
            }
            if (captureCheck == 1){
              displayWords = "Last Move:" + String(pieceMoved) + moveFrom + "x" + moveTo;
            }
            if (enPassantCheck == 1){
              displayWords = "Last Move:" + moveFrom + moveTo + "EP";
            }
            if (pawnPromotionMade == 1){
              displayWords = "Last Move:" + moveFrom + moveTo + "=Q";
            }
            writeToScreen(0, 1, displayWords);
            writeToScreen(0, 2, "Black's Turn    "); //Write to white screen, first line, "Black's Turn"
            postWords = "~" + displayWords;
            postTurn = "Black's Turn";
            postToPhant();
            turn = 'black';
            setBoard(0);
            pawnPromotionMade = 0;
        }
        noChangeFlag = 1;
     } else {
      noChangeFlag = 1;
      boardChange = 0;
     }

      readString = "";
      if (getPage() == 0){
        Serial.println("Get page failed");
      }
      delay(1000);
    } //else {
      //Serial.write("Error, invalid input from page");
      //Serial.println();
      //if (getPage() == 0){
        //Serial.println("Get page failed");
      //}
    //}
    
  } else if (physicalBoard == 1){ //black board
    read_shift_regs(1); //black
    checkButtons();
    //Update the LED on the board:
    setBoard(1); //black

    if (readString.endsWith("White's Turn")){
      turn = 0;
      Serial.println("Waiting for white to take turn");
      Serial.println(displayWords);
      readString = "";
      setBoard(1);
      removedPiece = 0;
      updatedLights = 0; 
      enPassantCheck = 0;
      writeToScreen(1, 1, displayWords);       //Write to black screen, first line, the last turn (from black)
      writeToScreen(1, 2, "White's Turn    "); //Write to black screen, second line, "White's Turn"
      if (getPage() == 0){
        Serial.println("Get page failed");
      }
      delay(5000);
    } else if (readString.endsWith("Black's Turn")){
      turn = 1;
      Serial.println(displayWords);

      
      if (displayWords.indexOf('White concedes') != -1){
        writeToScreen(1, 1, displayWords);
        writeToScreen(1, 2, "Black wins!     ");
        //winSequence(1);
        updatedLights = 1;
      } else if (displayWords.indexOf('Draw proposed') != -1){
        writeToScreen(1, 1, displayWords);
        writeToScreen(1, 2, "Y-Draw N-Enter  ");
        read_shift_regs(physicalBoard);
        while((buttonStates[0] = 1) && (buttonStates[1] == 1)){
          delay(500);
          read_shift_regs(physicalBoard);
        }
        if (buttonStates[0] == 0){
          writeToScreen(1, 1, "Draw denied     ");
          writeToScreen(1, 2, "sending decision");
          postWords = "~Draw denied     ";
          postTurn = "White's Turn";
          postToPhant();
          turn = 0;
          setBoard(1);
        } else if (buttonStates[1] == 0){
          writeToScreen(1, 1, "Draw accepted   ");
          writeToScreen(1, 2, "sending decision");
          postWords = "~Draw accepted   ";
          postTurn = "White's Turn";
          postToPhant();
          turn = 0;
          setBoard(1);
        }
        updatedLights = 1;
      } else if (displayWords.indexOf('Draw denied') != -1){
        writeToScreen(1, 1, "Draw denied     ");
        writeToScreen(1, 2, "Black's Turn    ");
        drawRequest = 0;
        updatedLights == 1;
      } else if (displayWords.indexOf('Draw accepted') != -1){
        writeToScreen(1, 1, "Draw accepted   ");
        writeToScreen(1, 2, "Game is a draw! ");
        //drawSquence(1);
        updatedLights = 1;
      } else if (displayWords.indexOf('New Game!') != -1){
        //It is the first turn, don't need position info
        firstTurn = 1;
      } else if (displayWords.indexOf('0-0-0') != -1){
        boardStatus[4] = 1;
        boardStatus[0] = 8;
        setBoard(1);
        writeToScreen(1, 1, displayWords);
        writeToScreen(1, 2, "Black's Turn    ");
        updatedLights = 1;
      } else if (displayWords.indexOf('0-0') != -1){
        boardStatus[4] = 2;
        boardStatus[7] = 8;
        setBoard(1);
        writeToScreen(1, 1, displayWords);
        writeToScreen(1, 2, "White's Turn    ");
        updatedLights = 1;
      } else if (displayWords.indexOf('EP') != -1){ //last move was an en Passant
        enPassantCheck = 1;
      } else if (displayWords.indexOf('=Q') != -1){
        pawnPromotionMade = 1;
      } else if (displayWords.indexOf('x') == -1){ //no capture 
        yourPieceCaptured = 0;
        getPosition = displayWords.substring(11);
        Serial.println(getPosition);
      } else if ((displayWords.indexOf('x') != -1) && (removedPiece == 0)){ 
        //Something was captured
        getPosition = displayWords.substring(10);
        Serial.println(getPosition);
        yourPieceCaptured = 1;
      }

      if (firstTurn == 1){ //first turn
        //don't need position info
        firstTurn = 0;
      } else if ((firstTurn == 0) && (updatedLights == 0)){
      //Parse the last move and update the board position array:
        updateNewPosition(1);
      }
      //Update the LED on the board:
      setBoard(1); //black
      writeToScreen(1, 1, displayWords);       //Write to white screen, first line, the last turn (from black)
      writeToScreen(1, 2, "Black's Turn    "); //Write to white screen, second line, "White's Turn"
      readString = "";
      Serial.println("Take your turn!");
      setBoard(1);
      read_shift_regs(1); //white
      checkButtons();



      int testArrays = arraysEqual(blackMagnetInput, oldBlackMagnetInput);
      Serial.write("Arrays equal? ");
      Serial.println(testArrays);
      Serial.write("Magnet input ");
      for (int i=0; i<DATA_WIDTH; i++){
        Serial.print(blackMagnetInput[i]);
      }
      Serial.println();
      Serial.write("Position info ");
      for (int i=0; i<DATA_WIDTH; i++){
        Serial.print(boardStatus[i]);
      }
      Serial.println();
      Serial.println(boardChange);
      
    if (testArrays == 0){
        //Allow the player some wait time to be able to change their mind:
        delay(pieceDelay);
        //Read the magnests again:
        read_shift_regs(1); //black
        checkButtons();
        //Check what piece moved from where to where, and if it captured anything:
        checkPiece(1); //black
        //Check if there was a full move made (picked piece up and put it back down):
        if (noChangeFlag == 0){
           //Update the oldWhiteMagnetInput array:
           for (int i = 0; i < DATA_WIDTH; i++){
              oldBlackMagnetInput[i] = blackMagnetInput[i];
            }
            //Write that it is black's turn on the LCD screen:
            findPositionName(0, moveFromNumber);
            findPositionName(1, moveToNumber);
            if (captureCheck == 0){
              displayWords = "Last Move: " + String(pieceMoved) + moveFrom + moveTo;
            }
            if (captureCheck == 1){
              displayWords = "Last Move:" + String(pieceMoved) + moveFrom + "x" + moveTo;
            }
            if (enPassantCheck == 1){
              displayWords = "Last Move:" + moveFrom + moveTo + "EP";
            }
            if (pawnPromotionMade == 1){
              displayWords = "Last Move:" + moveFrom + moveTo + "=Q";
            }
            writeToScreen(1, 1, displayWords);
            writeToScreen(1, 2, "White's Turn    "); //Write to black screen, first line, "White's Turn"
            postWords = "~" + displayWords;
            postTurn = "White's Turn";
            postToPhant();
            turn = 0;
            setBoard(1);
            pawnPromotionMade = 0;
        }
        noChangeFlag = 1;
     } else {
      noChangeFlag = 1;
      boardChange = 0;
     }

      readString = "";
      if (getPage() == 0){
        Serial.println("Get page failed");
      }
      delay(1000);
    } 
  }

  
  // If the server has disconnected, stop the client and try again
  if ( !client.connected() ) {
    Serial.println();
    Serial.write("Client disconnected");
    Serial.println();
    writeToScreen(physicalBoard, 1, "Disconnected    ");
    writeToScreen(physicalBoard, 2, "                ");
    // Close socket and wait for disconnect from WiFi
    client.stop();
    client.connect(http_site, http_port);
    postToPhant();
    if (getPage() == 0){
      Serial.println("Get page failed");
    }
  } 
}

