
/* 					ADPU Send format 		*/
00		// CLA
A4		// INS
04		// P1
00		// P2
0E		// LC	E = 14 DEC

/* DATA	*/
	32	50 	41 	59	2E	53	59 	53	2E	44	44	46	30	31	
//	1 	2 	3 	4 	5 	6 	7 	8 	9 	10 	11 	12 	13 	14 	15

00		// LE



/*					Received DATA 			*/
// Decimal Print	
/*	for (int i = 0; i < responseLength; i++) {
                Serial.print(response[i]);
                Serial.print(" ");
            }
*/
111 45 132 14 50 80 65 89 46 83 89 83 46 68 68 70 48 49 165 27 191 12 24 97 22 79 7 160 0 0 0 3 16 16 80 11 86 73 83 65 32 67 82 69 68 73 84 144 0 

//	Hex Dump
6F 2D 84 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 A5 1B BF 0C 18 61 16 4F 07 A0 00 00 00 03 10 10 50 0B 56 49 53 41 20 43 52 45 44 49 54 90 00
6F 2D 84 0E 32 50 41 59 2E 53 59 53 2E 44 44 46 30 31 A5 1B BF 0C 18 61 16 4F 07 A0 00 00 00 03 10 10 50 0B 56 49 53 41 20 43 52 45 44 49 54 90 00  o-�.2PAY.SYS.DDF01�.�..a.O.�......P.VISA CREDIT�.


//Tag
30 : Sequence
OC : UTF8 String
02 : Integer
01 : Boolean