
void RT_Button_init();
int RT_Button_Scan();

void RT_Button_Interrupt_init();

void IRAM_ATTR iRT();
void IRAM_ATTR iLT();
void IRAM_ATTR iUP();
void IRAM_ATTR iDN();
void IRAM_ATTR iA();
void IRAM_ATTR iB();
void IRAM_ATTR iST();
void IRAM_ATTR iSEL();


int upRead();
int encRead();
int encReadErase();
int encButtonRead();
bool getNeedMenu();
void setNeedMenu(bool);
void enableEncoderInturrupt();