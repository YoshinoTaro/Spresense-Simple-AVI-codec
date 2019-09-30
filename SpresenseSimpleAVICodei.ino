/*
 *  Spresense Simple AVI Codec :SpresenseSimpleAVICodec.ino
 *  Simple AVI codec for Sony Spresense
 *  Copyright 2019 Yoshino Taro
 * 
 *  This code is based on ArduCAM_Mini_Video2SD.ino
 *  Copyright 2017 Lee
 *  https://github.com/ArduCAM/Arduino
 *  http://www.ArduCAM.com
 *
 *  This sketch is made for Sony Spresense. 
 *  Other platforms are not supported. 
 *  You need Spresense boards below when you will try this sketch
 *    Spresense Main Board
 *    Spresense Camera Board
 *    Spresense Extension Board (for SD Card)
 *  See the detail: https://developer.sony.com/develop/spresense/
 *
 *  The license of this source code is LGPL v2.1 
 *  (because Arduino library of Spresense is under LGPL v2.1)
 *
 */

#include <Camera.h>
#include <SDHCI.h>
#include <stdio.h>
#include <math.h>

SDClass theSD;

/* WIDTH == 1280 (0x500) */
#define WIDTH_1 0x00
#define WIDTH_2 0x05
/* HEIGHT == 960 (0x3C0) */
#define HEIGHT_1 0xC0
#define HEIGHT_2 0x03
#define TOTAL_FRAMES 300
#define AVIOFFSET 240

unsigned long movi_size = 0;
const char avi_header[AVIOFFSET+1] = {
  0x52, 0x49, 0x46, 0x46, 0xD8, 0x01, 0x0E, 0x00, 0x41, 0x56, 0x49, 0x20, 0x4C, 0x49, 0x53, 0x54,
  0xD0, 0x00, 0x00, 0x00, 0x68, 0x64, 0x72, 0x6C, 0x61, 0x76, 0x69, 0x68, 0x38, 0x00, 0x00, 0x00,
  0xA0, 0x86, 0x01, 0x00, 0x80, 0x66, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  WIDTH_1, WIDTH_2, 0x00, 0x00, HEIGHT_1, HEIGHT_2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x84, 0x00, 0x00, 0x00,
  0x73, 0x74, 0x72, 0x6C, 0x73, 0x74, 0x72, 0x68, 0x30, 0x00, 0x00, 0x00, 0x76, 0x69, 0x64, 0x73,
  0x4D, 0x4A, 0x50, 0x47, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x66,
  0x28, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00, WIDTH_1, WIDTH_2, 0x00, 0x00, HEIGHT_1, HEIGHT_2, 0x00, 0x00,
  0x01, 0x00, 0x18, 0x00, 0x4D, 0x4A, 0x50, 0x47, 0x00, 0x84, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54,
  0x10, 0x00, 0x00, 0x00, 0x6F, 0x64, 0x6D, 0x6C, 0x64, 0x6D, 0x6C, 0x68, 0x04, 0x00, 0x00, 0x00,
  0x64, 0x00, 0x00, 0x00, 0x4C, 0x49, 0x53, 0x54, 0x00, 0x01, 0x0E, 0x00, 0x6D, 0x6F, 0x76, 0x69,
  0x00
};

File aviFile;
String filename = "movie.avi";
uint32_t start_ms = 0;

static void inline uint32_write_to_aviFile(uint32_t v) { 
  char value = v % 0x100;
  aviFile.write(value);  v = v >> 8; 
  value = v % 0x100;
  aviFile.write(value);  v = v >> 8;
  value = v % 0x100;
  aviFile.write(value);  v = v >> 8; 
  value = v;
  aviFile.write(value);
}


void setup() {
  Serial.begin(115200);

  theCamera.begin();
  theSD.begin();

  theSD.remove(filename);
  aviFile = theSD.open(filename ,FILE_WRITE);
  aviFile.write(avi_header, AVIOFFSET);

  Serial.println("Recording...");

  theCamera.setStillPictureImageFormat(
     CAM_IMGSIZE_QUADVGA_H,
     CAM_IMGSIZE_QUADVGA_V,
     CAM_IMAGE_PIX_FMT_JPG);

  start_ms = millis();
  digitalWrite(LED0 ,HIGH);
}

int loopCounter = 0;
void loop() {
  
  CamImage img = theCamera.takePicture();
  if (!img.isAvailable()) {
    Serial.println("faile to take a picture");
    return;
  }
  
  aviFile.write("00dc", 4);

  uint32_t jpeg_size = img.getImgSize();
  uint32_write_to_aviFile(jpeg_size);
  
  aviFile.write(img.getImgBuff() ,jpeg_size);
  movi_size += jpeg_size;

  /* Spresense's jpg file is assumed to be 16bits aligned 
   * So, there's no padding operation */

  if (++loopCounter == TOTAL_FRAMES) {
    float duration_sec = (millis() - start_ms) / 1000.0f;
    float fps_in_float = loopCounter / duration_sec;
    float us_per_frame_in_float = 1000000.0f / fps_in_float;
    uint32_t fps = round(fps_in_float);
    uint32_t us_per_frame = round(us_per_frame_in_float);

    /* overwrite riff file size */
    aviFile.seek(0x04);
    uint32_t total_size = movi_size + 12*loopCounter + 4;
    uint32_write_to_aviFile(total_size);

    /* overwrite hdrl */
    /* hdrl.avih.us_per_frame */
    aviFile.seek(0x20);
    uint32_write_to_aviFile(us_per_frame);
    uint32_t max_bytes_per_sec = movi_size * fps / loopCounter;
    aviFile.seek(0x24);
    uint32_write_to_aviFile(max_bytes_per_sec);

    /* hdrl.avih.tot_frames */
    aviFile.seek(0x30);
    uint32_write_to_aviFile(loopCounter);
    aviFile.seek(0x84);
    uint32_write_to_aviFile(fps);   

    /* hdrl.strl.list_odml.frames */
    aviFile.seek(0xe0);
    uint32_write_to_aviFile(loopCounter);
    aviFile.seek(0xe8);
    uint32_write_to_aviFile(movi_size);

    aviFile.close();

    Serial.println("Movie saved");
    Serial.println(" File size (kB): " + String(total_size));
    Serial.println(" Captured Frame: " + String(loopCounter)); 
    Serial.println(" Duration (sec): " + String(duration_sec));
    Serial.println(" Frame per sec : " + String(fps));
    Serial.println(" Max data rate : " + String(max_bytes_per_sec));

    digitalWrite(LED0, LOW);
    while(1);

  }
}
