/* HV1 packet */
int bluetooth_sniffer::HV1(char *stream)
{
	int length;
	length = 10;
	printf(" Length:%d\nUAP could not be confirmed by payload\n", length);
	stream = unfec13(stream, length*8);
	d_payload_size = length;
	return 0;
}

/* HV2 packet */
int bluetooth_sniffer::HV2(char *stream)
{
	int length;
	length = 20;
	printf(" Length:%d\nUAP could not be confirmed by payload\n", length);
	stream = unfec23(stream, length*8);
	d_payload_size = length;
	return 0;
}

/* HV3 packet */
int bluetooth_sniffer::HV3(char *stream)
{
	int length;
	length = 30;
	printf(" Length:%d\nUAP could not be confirmed by payload\n", length);
	d_payload_size = length;
	return 0;
}

/* DV packet */
int bluetooth_sniffer::DV(char *stream, int UAP, int size)
{
	int length, count;
	uint16_t crc, check;

	length = payload_header(stream);
	printf(" Length of data field:%d\n", length);

	/*Un-FEC 2/3 it */
	unfec23(stream+81, (length+2)*8);

	length++;
	size -= 80;
	size -= 8*(length+2);

	if(0 > size)
		return 1;

	for(count = 0; count < length+2; count++)
		stream[count+80] = gr_to_normal(stream+(8*count)+80);

	crc = crcgen(stream+80, length, UAP);

	check = stream[length+81] | stream[length+80] << 8;
	if(crc != check)
		printf("ERROR: UAPs do not match\n");
	else
		printf("UAP confirmed by CRC check\n");

	d_payload_size = length + 9;
	return 0;
}

/* EV3 packet */
int bluetooth_sniffer::EV3(char *stream, int UAP, int size)
{
	return 0;
}

/* EV4 packet */
int bluetooth_sniffer::EV4(char *stream, int UAP, int size)
{
	return 0;
}

/* EV5 packet */
int bluetooth_sniffer::EV5(char *stream, int UAP, int size)
{
	return 0;
}

/* DM1 packet */
int bluetooth_sniffer::DM1(char *stream, int UAP, int size)
{
	int length, count;
	uint16_t crc, check;	
	length = 0;

	if(8 >= size)
		return 1;

	length = payload_header(stream);

	/*Un-FEC 2/3 it */
	unfec23(stream+1, (length+2)*8);

	length++;
	size -= 8*(length+2);

	if(0 > size)
		return 1;

	for(count = 0; count < length+2; count++)
		stream[count] = gr_to_normal(stream+(8*count));

	crc = crcgen(stream, length, UAP);

	check = stream[length+1] | stream[length] << 8;
	if(crc != check)
		printf("ERROR: UAPs do not match\n");
	else
		printf("UAP confirmed by CRC check\n");

	d_payload_size = length - 1;
	return 0;
}

/* DH1 packet */
int bluetooth_sniffer::DH1(char *stream, int UAP, int size)
{
	int count;
	uint16_t crc, check, length;	
	length = 0;

	if(8 >= size)
		return 1;

	length = payload_header(stream);
	length++;
	size -= 8*(length+2);

	if(0 > size)
		return 1;

	for(count = 0; count < length+2; count++)
		stream[count] = gr_to_normal(stream+(8*count));

	crc = crcgen(stream, length, UAP);
	check = stream[length+1] | stream[length] << 8;

	if(crc != check)
		printf("\nERROR: CRCs do not match 0x%04x != 0x%02x%02x\n", crc, stream[length], stream[length+1]);
	else
		printf("\nUAP confirmed by CRC check\n");

	d_payload_size = length - 1;
	return 0;
}

/* DM3 packet */
int bluetooth_sniffer::DM3(char *stream, int UAP, int size)
{
	int length, count;
	uint16_t crc, check;	
	length = 0;

	if(8 >= size)
		return 1;

	length = long_payload_header(stream);

	/*Un-FEC 2/3 it */
	unfec23(stream+1, (length+2)*8);

	length += 2;
	size -= 8*(length+2);

	if(0 > size)
		return 1;

	for(count = 0; count < length+2; count++)
		stream[count] = gr_to_normal(stream+(8*count));

	crc = crcgen(stream, length, UAP);

	check = stream[length+1] | stream[length] << 8;
	if(crc != check)
		printf("ERROR: UAPs do not match\n");
	else
		printf("UAP confirmed by CRC check\n");

	d_payload_size = length - 1;
	return 0;
}

/* DH3 packet */
int bluetooth_sniffer::DH3(char *stream, int UAP, int size)
{
	int length, count;
	uint16_t crc, check;	
	length = 0;

	if(8 >= size)
		return 1;

	length = long_payload_header(stream);
	length += 2;
	size -= 8*(length+2);

	if(0 > size)
		return 1;

	for(count = 0; count < length+2; count++)
		stream[count] = gr_to_normal(stream+(8*count));

	crc = crcgen(stream, length, UAP);
	check = stream[length+1] | stream[length] << 8;

	if(crc != check)
		printf("ERROR: UAPs do not match\n");
	else
		printf("UAP confirmed by CRC check\n");

	d_payload_size = length - 1;
	return 0;
}

/* AUX1 packet */
int bluetooth_sniffer::AUX1(char *stream, int size)
{
	int length;

	if(8 >= size)
		return 1;

	length = payload_header(stream);
	printf(" Length:%d\n", length);
	length++;

	d_payload_size = length;
	return 0;
}

/* POLL packet */
int bluetooth_sniffer::POLL()
{
	printf(" No payload\n");
	return 0;
}

/* FHS packet */
int bluetooth_sniffer::FHS(char *stream, int UAP)
{
	int length, payload_UAP, NAP;
	length = 10;
	stream = unfec23(stream, length*8);

	payload_UAP = stream[64] + stream[65] + stream[66] + stream[67] + stream[68] + stream[69] +stream[70] +stream[71];

	NAP = stream[72] + stream[73] + stream[74] + stream[75] + stream[76] + stream[77] + stream[78] + stream[79] + stream[80] + stream[81] + stream[82] + stream[83] + stream[84] + stream[85] + stream[86] + stream[87];

	if(UAP == payload_UAP)
	{
		printf("\nNAP:%04x clk:", NAP);
		printf("%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d\n", stream[115], stream[116], stream[117], stream[118], stream[119], stream[120], stream[121], stream[122], stream[123], stream[124], stream[125], stream[126], stream[127], stream[128], stream[129], stream[130], stream[131], stream[132], stream[133], stream[134], stream[135], stream[136], stream[137], stream[138], stream[139]);
	} else {
		printf("\nUAPs don't match\n");
	}
	d_payload_size = length;
	return 0;
}

/* NULL packet */
int bluetooth_sniffer::null_packet()
{
	printf(" No payload\n");
	d_payload_size = 0;
	return 0;
}
