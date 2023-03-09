#include "USB.h"
#include "USBMSC.h"
#include "FFat.h"
#include "FS.h"

// Block size of flash memory (in bytes) (4KB)
#define BLOCK_SIZE 4096

// USB Mass Storage Class (MSC) object
USBMSC msc;

// Flash memory object
EspClass _flash;

// Partition information object
const esp_partition_t* Partition;


const esp_partition_t* partition(){
  // Return the first SPIFFS partition found (should be the only one)
  return esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
}


static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize){
  //Serial.println("Write lba: " + String(lba) + " offset: " + String(offset) + " bufsize: " + String(bufsize));

  // Erase block before writing as to not leave any garbage
  _flash.partitionEraseRange(Partition, offset + (lba * BLOCK_SIZE), bufsize);

  // Write data to flash memory in blocks from buffer
  _flash.partitionWrite(Partition, offset + (lba * BLOCK_SIZE), (uint32_t*)buffer, bufsize);

  return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize){
  //Serial.println("Read lba: " + String(lba) + " offset: " + String(offset) + " bufsize: " + String(bufsize));

  // Read data from flash memory in blocks and store in buffer
  _flash.partitionRead(Partition, offset + (lba * BLOCK_SIZE), (uint32_t*)buffer, bufsize);

  return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject){
  return true;
}


void setup(){
  Serial.begin(115200);

  Serial.println("Starting Serial");


  Serial.println("Getting partition info");
  // Get partition information
  Partition = partition();


  Serial.println("Initializing MSC");
  // Initialize USB metadata and callbacks for MSC (Mass Storage Class)
  msc.vendorID("ESP32");
  msc.productID("USB_MSC");
  msc.productRevision("1.0");
  msc.onRead(onRead);
  msc.onWrite(onWrite);
  msc.onStartStop(onStartStop);
  msc.mediaPresent(true);
  msc.begin(Partition->size/BLOCK_SIZE, BLOCK_SIZE);

  Serial.println("Initializing USB");

  USB.begin();


  Serial.println("Initializing FFat");

  // begin(true) will format on fail
  if(!FFat.begin(true)){
    Serial.println("Mount Failed");
  }else{
    Serial.println("fat succsess");
  }

  // Windows will format the flash as FAT filesystem if it is not already formatted

  Serial.println("Listing files");

  // Print directory contents
  listDir(FFat, "/", 0);

  Serial.println("Printing flash size");

  // Print flash size
  char buff[50];
  sprintf(buff, "Flash Size: %d", Partition->size);
  Serial.println(buff);
}

void loop(){
  delay(2000);
}


void listDir(fs::FS& fs, const char* dirname, uint8_t outshoot) {
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      for (int i = 0; i < outshoot; i++) {
        Serial.println("  ");
      }
      //Serial.print("  DIR : ");
      Serial.println(file.name());
      Serial.println("\n");

    } else {
      for (int i = 0; i < outshoot; i++) {
        Serial.println("  ");
      }
      Serial.println(file.name());
      Serial.println("\n");
    }
    file = root.openNextFile();
  }
}
