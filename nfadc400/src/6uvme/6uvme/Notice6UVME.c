#include "Notice6UVME.h"

enum EManipInterface {kInterfaceClaim, kInterfaceRelease};

struct dev_open {
   libusb_device_handle *devh;
   uint16_t vendor_id;
   uint16_t product_id;
   int serial_id;
   struct dev_open *next;
} *ldev_open = 0;

// internal functions *********************************************************************************
static void add_device(struct dev_open **list, libusb_device_handle *tobeadded);
static int handle_interface_id(struct dev_open **list, int interface, enum EManipInterface manip_type);
static void remove_device_id(struct dev_open **list);
libusb_device_handle* nkusb_get_device_handle(void);
int Vwrite(unsigned char am, unsigned char tout, unsigned long address, unsigned long data);
int Vread(unsigned char am, unsigned char tout, unsigned long address, unsigned long *data);
int Vblockread(unsigned short am, unsigned short tout, unsigned long address, unsigned long count, char *data);

static void add_device(struct dev_open **list, libusb_device_handle *tobeadded)
{
  struct dev_open *curr;

  curr = (struct dev_open *)malloc(sizeof(struct dev_open));
  curr->devh = tobeadded;
  curr->vendor_id = VME_VENDOR_ID;
  curr->product_id = VME_PRODUCT_ID;
  curr->serial_id = 1;
  curr->next  = *list;
  *list = curr;
}

static int handle_interface_id(struct dev_open **list, int interface, enum EManipInterface manip_type)
{
  int ret = 0;
  if (!*list) 
    return -1;

  struct dev_open *curr = *list;
  struct libusb_device_descriptor desc;
  libusb_device *dev;

  while (curr) {
    dev =libusb_get_device(curr->devh);
    libusb_get_device_descriptor(dev, &desc); 

    if (desc.idVendor == VME_VENDOR_ID && desc.idProduct == VME_PRODUCT_ID) {
      if (manip_type == kInterfaceClaim) 
        ret = libusb_claim_interface(curr->devh, interface);
      else if (manip_type == kInterfaceRelease) 
        ret =libusb_release_interface(curr->devh, interface);
      else 
        return -1;
    }

    curr = curr->next;
  }

  return ret;
}

static void remove_device_id(struct dev_open **list)
{
  if (!*list) return;

  struct dev_open *curr = *list;
  struct dev_open *prev = 0;
  struct libusb_device_descriptor desc;
  libusb_device *dev;

  while (curr) {
    dev =libusb_get_device(curr->devh);
    libusb_get_device_descriptor(dev, &desc);

    if (desc.idVendor == VME_VENDOR_ID && desc.idProduct == VME_PRODUCT_ID) { 
      if (*list == curr) { 
        *list = curr->next;
        libusb_close(curr->devh);
        free(curr); 
        curr = *list;
      }
      else {
        prev->next = curr->next;
        libusb_close(curr->devh);
        free(curr); 
        curr = prev->next;
      }
    }
    else {
      prev = curr;
      curr = curr->next;
    }    
  }
}

libusb_device_handle* nkusb_get_device_handle(void) 
{
  struct dev_open *curr = ldev_open;
  while (curr) {
    if (curr->vendor_id == VME_VENDOR_ID && curr->product_id == VME_PRODUCT_ID) 
      return curr->devh;

    curr = curr->next;
  }

  return 0;
}

// write cycle
int Vwrite(unsigned char am, unsigned char tout, unsigned long address, unsigned long data)
{
  int transferred = 0;  
  const unsigned int timeout = 1000;
  int length = 12;
  unsigned char *buffer;

  if (!(buffer = (unsigned char *)malloc(length))) 
    return -1;

  buffer[0] = 0x00;                                // can be any value
  buffer[1] = 0x00;                                // can be any value
  buffer[2] = am & 0xFF;                           // WRITE+LWORD+AM
  buffer[3] = tout & 0xFF;                         // timeout in us
  buffer[4] = address & 0xFF;                      // address
  buffer[5] = (address >> 8) & 0xFF;
  buffer[6] = (address >> 16) & 0xFF;
  buffer[7] = (address >> 24) & 0xFF;
  buffer[8] = data & 0xFF;                         // data
  buffer[9] = (data >> 8) & 0xFF;
  buffer[10] = (data >> 16) & 0xFF;
  buffer[11] = (data >> 24) & 0xFF;

  libusb_device_handle *devh = nkusb_get_device_handle();
  if (!devh) 
    return -1;
  
  if (libusb_bulk_transfer(devh, USB3_SF_WRITE, buffer, length, &transferred, timeout) < 0) {
    free(buffer);
    return -1;
  }

  free(buffer);

  usleep(250);

  return 0;
}

// read cycle
int Vread(unsigned char am, unsigned char tout, unsigned long address, unsigned long *data)
{
  const unsigned int timeout = 1000; 
  int length = 12;
  int transferred;
  unsigned char *buffer;
  unsigned char vmemode;
  unsigned char lword;
  unsigned long tmp;

  if (!(buffer = (unsigned char *)malloc(length))) 
    return -1;

  vmemode = am | 0x80;                         // set in read mode
  lword = vmemode & 0x40;           

  buffer[0] = 0x00;                                // can be any value
  buffer[1] = 0x00;                                // can be any value
  buffer[2] = vmemode & 0xFF;                      // WRITE+LWORD+AM
  buffer[3] = tout & 0xFF;                         // timeout in us
  buffer[4] = address & 0xFF;                      // start address
  buffer[5] = (address >> 8) & 0xFF;
  buffer[6] = (address >> 16) & 0xFF;
  buffer[7] = (address >> 24) & 0xFF;

  if (lword) {
    buffer[8] = 2;                     
    buffer[9] = 0;
    buffer[10] = 0;
    buffer[11] = 0;
  }
  else {
    buffer[8] = 4;                     
    buffer[9] = 0;
    buffer[10] = 0;
    buffer[11] = 0;
  }

  libusb_device_handle *devh = nkusb_get_device_handle();
  if (!devh) 
    return -1;

  if (libusb_bulk_transfer(devh, USB3_SF_WRITE, buffer, length, &transferred, timeout) < 0) {
    free(buffer);
    return -1;
  }

  if (lword) {
    if (libusb_bulk_transfer(devh, USB3_SF_READ, buffer, 2, &transferred, timeout) < 0) 
      return -1;

    data[0] = buffer[0] & 0xFF;
    tmp = buffer[1] & 0xFF;
    tmp = tmp << 8;
    data[0] = data[0] + tmp;
  }
  else {
    if (libusb_bulk_transfer(devh, USB3_SF_READ, buffer, 4, &transferred, timeout) < 0) 
      return -1;

    data[0] = buffer[0] & 0xFF;
    tmp = buffer[1] & 0xFF;
    tmp = tmp << 8;
    data[0] = data[0] + tmp;
    tmp = buffer[2] & 0xFF;
    tmp = tmp << 16;
    data[0] = data[0] + tmp;
    tmp = buffer[3] & 0xFF;
    tmp = tmp << 24;
    data[0] = data[0] + tmp;
  }

  free(buffer);
  
  return 0;
}

// block read cycle
int Vblockread(unsigned short am, unsigned short tout, unsigned long address, unsigned long count, char *data)
{
  const unsigned int timeout = 1000; 
  int length = 12;
  int transferred;
  unsigned char *buffer;
  int size = 1048576; // 1 Mbyte
  int nbulk;
  int remains;
  int loop;
  unsigned char vmemode;
  
  nbulk = count / 1048576;
  remains = count % 1048576;

  if (!(buffer = (unsigned char *)malloc(size))) {
    fprintf(stderr, "USB3Read: Could not allocate memory (size = %d\n)", size);
    return -1;
  }
  
  vmemode = am | 0x80;                             // set in read mode
  buffer[0] = 0x00;                                // can be any value
  buffer[1] = 0x00;                                // can be any value
  buffer[2] = vmemode & 0xFF;                      // WRITE+LWORD+AM
  buffer[3] = tout & 0xFF;                         // timeout in us
  buffer[4] = address & 0xFF;                      // start address
  buffer[5] = (address >> 8) & 0xFF;
  buffer[6] = (address >> 16) & 0xFF;
  buffer[7] = (address >> 24) & 0xFF;
  buffer[8] = count & 0xFF;                      
  buffer[9] = (count >> 8) & 0xFF;
  buffer[10] = (count >> 16) & 0xFF;
  buffer[11] = (count >> 24) & 0xFF;

  libusb_device_handle *devh = nkusb_get_device_handle();
  if (!devh) {
    fprintf(stderr, "USB3Write: Could not get device handle for the device.\n");
    return -1;
  }

  if (libusb_bulk_transfer(devh, USB3_SF_WRITE, buffer, length, &transferred, timeout) < 0) {
    free(buffer);
    return -1;
  }

  for (loop = 0; loop < nbulk; loop++) {
    if (libusb_bulk_transfer(devh, USB3_SF_READ, buffer, size, &transferred, timeout) < 0) 
      return -1;
    memcpy(data + loop * size, buffer, size);
  }

  if (remains) {
    if (libusb_bulk_transfer(devh, USB3_SF_READ, buffer, remains, &transferred, timeout) < 0) 
      return 1;
    memcpy(data + nbulk * size, buffer, remains);
  }

  free(buffer);
  
  return 0;
}

//*********** end of internal functions *********************************************************

// open VME
int VMEopen(void)
{
  struct libusb_device **devs;
  struct libusb_device *dev;
  struct libusb_device_handle *devh;
  struct libusb_device_descriptor desc;
  size_t i = 0;
  int nopen_devices = 0; 
  int interface = 0;
  int r;

  if (libusb_init(0) < 0) 
    exit(1);
    
  libusb_get_device_list(0, &devs);

  while ((dev = devs[i++])) {
    libusb_get_device_descriptor(dev, &desc);

    if (desc.idVendor == VME_VENDOR_ID && desc.idProduct == VME_PRODUCT_ID)  {
printf("%X : %X module found!\n", desc.idVendor, desc.idProduct);    
      r = libusb_open(dev, &devh);
      if (r < 0) {
        fprintf(stdout, "Warning, open_device: could not open device." " Ignoring.\n");
        continue;
      }

      if (libusb_claim_interface(devh, interface) < 0) {
        printf("Warning, open_device: could not claim interface 0 on the device." " Ignoring.\n");
        libusb_close(devh);
        return 0;
      }

      add_device(&ldev_open, devh);
      nopen_devices++;
  
      libusb_release_interface(devh, interface);
    }
  }

  libusb_free_device_list(devs, 1);

  handle_interface_id(&ldev_open, 0, kInterfaceClaim);

  if (!nopen_devices)
    return 0;

  devh = nkusb_get_device_handle();
  printf("device handle = %d\n", devh);  
  if (!devh) 
    return 0;

  return 1;
}

// close VME
void VMEclose(void)
{
  handle_interface_id(&ldev_open, 0, kInterfaceRelease);
  remove_device_id(&ldev_open);
  libusb_exit(0); 
}


// VME write cycle
int VMEwrite(unsigned short am, unsigned short tout, unsigned long address, unsigned long data)
{
  int flag;
  int ntry;

  flag = -1;
  ntry = 0;

  while (flag < 0) {
    if(ntry > 5) {
      printf("VMEwrite does not respond. After 5 try, giving up! \n");
      return -1;
    }
    ntry++;
    flag = Vwrite(am, tout, address, data);  // send command to EP6
    
    if (flag < 0) {
      printf("VMEwrite does not respond. Close and Open VME again \n");
      VMEclose();
      VMEopen();
    }
  }

  return 0;
}


// VME read cycle
unsigned long VMEread(unsigned short am, unsigned short tout, unsigned long address)
{
  unsigned long data[2];
  int flag;
  int ntry;

  flag = -1;
  ntry =0;

  while (flag < 0) {
    if(ntry > 5) {
      printf("VMEread does not respond. After 5 try, giving up! \n");
      return -1;      
    }
    ntry++;
    flag = Vread(am, tout, address, data);       // send command to EP6

    if (flag < 0) {
      printf("VMEread does not respond. Close and Open VME again \n");
      VMEclose();
      VMEopen();
    }
  }

  return data[0];
}
  

// VME block read cycle
int VMEblockread(unsigned short am, unsigned short tout, unsigned long address, unsigned long count, char *data)
{
  int flag;
  int ntry;

  flag = -1;
  ntry = 0;

  while (flag < 0) {
    if(ntry > 5) {
      printf("VMEbloakread does not respond. After 5 try, giving up! \n");
      return -1;
    }
    ntry++;
    flag = Vblockread(am, tout, address, count, data);
    
    if (flag < 0) {
      VMEclose();
      VMEopen();
      printf("VMEblockread does not respond. Close and Open VME again \n");
    }
  }

  return 0;
}



