// Taken from the gssm project (http://www.thre.at/gsm)
unsigned short HCI_H1 = 0xFFFD;
unsigned short HCI_H4 = 0xFFFE;

int mktun(const char *, unsigned char *);
int write_interface(int, unsigned char *, unsigned int, uint64_t, uint64_t, unsigned short);
