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


void listDir(fs::FS& fs, const char* dirname, int numTabs = 0);


const esp_partition_t* partition(){
  // Return the first FAT partition found (should be the only one)
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

  Serial.println("Initializing FFat");

  // False for the msc mode and True for internal filesystem mode
  // Since both cannot write to the same table at the same time
  if (true){

    // begin(true) will format on fail 
    if(!FFat.begin()){
      Serial.println("Mount Failed, formatting...");

      // If mount fails, format the partition (Feels cleaner than FFat.begin(true))
      if(FFat.format(FFAT_WIPE_FULL)){
        Serial.println("Format Success");
      } else {
        Serial.println("Format Failed");
      }
    }else{
      Serial.println("fat success");
    }

    Serial.println("Listing before");

    listDir(FFat, "/");

    Serial.println("Creating file");

    // Create a file | use F("...") on file paths or it creates weird problems (idk why)
    File f = FFat.open(F("/test.txt"), FILE_WRITE, true);

    // Write to file (using F("...") here just to be safe) 
    f.println(F("Test"));

    // Close and flush file (Not sure if flush is needed, but there to be safe)
    f.flush();
    f.close();

    Serial.println("Listing After");

    // Print directory contents
    listDir(FFat, "/");

    
  } else {
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

    Serial.println("Printing flash size");

    //Print flash size
    char buff[50];
    sprintf(buff, "Flash Size: %d", Partition->size);
    Serial.println(buff);
  }
}

void loop(){
  delay(5000);
}


void listDir(fs::FS& fs, const char* dirname, int numTabs) {
  File dir = fs.open(F(dirname));

  if(!dir){
    Serial.print("Failed to open directory: ");
    Serial.println(dirname);
    return;
  }
  if(!dir.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  while (true) {
    File file =  dir.openNextFile();
    if (!file) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(file.name());
    if (file.isDirectory()) {
      Serial.println("/");
      // Ugly string concatenation to get the full path
      listDir(fs, ((std::string)"/" + (std::string)file.name()).c_str(), numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t\tSize: ");
      Serial.println(file.size(), DEC);
    }
    file.close();
  }
}
