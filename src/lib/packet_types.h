/* HV1 packet */
int bluetooth_sniffer::HV1(char *stream);


/* HV2 packet */
int bluetooth_sniffer::HV2(char *stream);


/* HV3 packet */
int bluetooth_sniffer::HV3(char *stream);


/* DV packet */
int bluetooth_sniffer::DV(char *stream, int UAP, int size);


/* EV3 packet */
int bluetooth_sniffer::EV3(char *stream, int UAP, int size);


/* EV4 packet */
int bluetooth_sniffer::EV4(char *stream, int UAP, int size);


/* EV5 packet */
int bluetooth_sniffer::EV5(char *stream, int UAP, int size);


/* DM1 packet */
int bluetooth_sniffer::DM1(char *stream, int UAP, int size);


/* DH1 packet */
int bluetooth_sniffer::DH1(char *stream, int UAP, int size);


/* DM3 packet */
int bluetooth_sniffer::DM3(char *stream, int UAP, int size);


/* DH3 packet */
int bluetooth_sniffer::DH3(char *stream, int UAP, int size);


/* AUX1 packet */
int bluetooth_sniffer::AUX1(char *stream, int size);


/* POLL packet */
int bluetooth_sniffer::POLL();


/* FHS packet */
int bluetooth_sniffer::FHS(char *stream, int UAP);


/* NULL packet */
int bluetooth_sniffer::null_packet();

