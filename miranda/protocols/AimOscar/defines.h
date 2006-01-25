#ifndef DEFINES_H
#define DEFINES_H
#define KEEPALIVE_TIMER					1000*60*7// 1000 milliseconds * 60 = 60 secs
#define MSG_LEN							4089
#define FLAP_SIZE						6
#define SNAC_SIZE						10
#define TLV_HEADER_SIZE					4
#define TLV_PART_SIZE					2
#define AIM_KEY_PW						"password"
#define AIM_KEY_HN						"hostname"
#define AIM_KEY_DC						"DelConf"//delivery confirmation
#define AIM_KEY_FP						"ForceProxyTransfer"
#define AIM_KEY_GP						"FileTransferGracePeriod"//in seconds default 60
#define AIM_KEY_ST						"Status"
#define AIM_KEY_SN						"SN"
#define AIM_KEY_NK						"Nick"
#define AIM_KEY_BI						"BuddyId"
#define AIM_KEY_GI						"GroupId"
#define AIM_MOD_IG						"ID2Group"
#define AIM_MOD_GI						"Group2ID"
#define AIM_KEY_SM						"StatusMsg"
#define AIM_KEY_IT						"IdleTS"
#define AIM_KEY_OT						"LogonTS"
#define AIM_KEY_NB						"NewBuddy"
#define AIM_KEY_FT						"FileTransfer"//1= sending 0=receiving
#define AIM_KEY_CK						"Cookie"
#define AIM_KEY_CK2						"Cookie2"
#define AIM_KEY_FN						"FileName"
#define AIM_KEY_FS						"FileSize"
#define AIM_KEY_FD						"FileDesc"
#define AIM_KEY_IP						"IP"
#define AIM_KEY_PS						"ProxyStage"
#define AIM_KEY_PC						"PortCheck"
#define AIM_KEY_DH						"DCHandle"
#define AIM_DEFAULT_GROUP				"Miranda Merged"
#define AIM_MD5_STRING					"AOL Instant Messenger (SM)"
#define AIM_DEFAULT_SERVER				"login.oscar.aol.com:5190"
#define MD5_HASH_LENGTH					16
#define AIM_CLIENT_ID_STRING			"Miranda Oscar Plugin, version 0.0.0.1"
#define AIM_CLIENT_ID_NUMBER			"\x01\x09"
#define AIM_CLIENT_MAJOR_VERSION		"\0\x05"
#define AIM_CLIENT_MINOR_VERSION		"\0\x09"
#define AIM_CLIENT_LESSER_VERSION		"\0\0"
#define AIM_CLIENT_BUILD_NUMBER			"\x0b\xdc"
#define AIM_CLIENT_DISTRIBUTION_NUMBER	"\0\0\0\xd2"
#define AIM_LANGUAGE					"en"
#define AIM_COUNTRY						"us"
#define AIM_MSG_TYPE					"text/x-aolrtf; charset=\"us-ascii\""
#define AIM_TOOL_VERSION				"\x01\x10\x08\xf1"
#define AIM_CAPS_LENGTH					16
#define AIM_CAP_VOICE_CHAT				"\x09\x46\x13\x41\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_DIRECT_PLAY				"\x09\x46\x13\x42\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_SEND_FILES				"\x09\x46\x13\x43\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_DIRECT_IM				"\x09\x46\x13\x45\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_AVATARS					"\x09\x46\x13\x46\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_STOCKS					"\x09\x46\x13\x47\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_RECEIVE_FILES			"\x09\x46\x13\x48\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_GAMES					"\x09\x46\x13\x4A\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_LIST_TRANSFER			"\x09\x46\x13\x4B\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_ICQ_SUPPORT				"\x09\x46\x13\x4D\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_UTF8					"\x09\x46\x13\x4E\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_DIRECT_PLAY				"\x09\x46\x13\x42\x4C\x7F\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_CHAT					"\x74\x8F\x24\x20\x62\x87\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_ICHAT					"\x09\x46\x00\x00\x4c\x7f\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_UNKNOWN					"\x09\x46\x01\x05\x4c\x7f\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_CAP_UNKNOWN3				"\x09\x46\x01\x03\x4c\x7f\x11\xD1\x82\x22\x44\x45\x53\x54\0\0"
#define AIM_SERVICE_GENERIC				"\0\x01\0\x04"//version 4
#define AIM_SERVICE_SSI					"\0\x13\0\x03"//version 3
#define AIM_SERVICE_LOCATION			"\0\x02\0\x01"//version 1
#define AIM_SERVICE_BUDDYLIST			"\0\x03\0\x01"//version 1
#define AIM_SERVICE_MESSAGING			"\0\x04\0\x01"//version 1
#define AIM_SERVICE_INVITATION			"\0\x06\0\x01"//version 1
#define AIM_SERVICE_POPUP				"\0\x08\0\x01"//version 1
#define AIM_SERVICE_BOS					"\0\x09\0\x01"//version 1
#define AIM_SERVICE_USERLOOKUP			"\0\x0A\0\x01"//version 1
#define AIM_SERVICE_STATS				"\0\x0B\0\x01"//version 1
#define AIM_SERVICE_RATES				"\0\x01\0\x02\0\x03\0\x04\0\x05"
#define AIM_STATUS_WEBAWARE				"\0\x01"	
#define AIM_STATUS_SHOWIP				"\0\x02"
#define AIM_STATUS_BIRTHDAY				"\0\x08"
#define AIM_STATUS_WEBFRONT				"\0\x20"
#define AIM_STATUS_DCAUTH				"\x10\0"
#define AIM_STATUS_DCCONT				"\x20\0"
#define AIM_STATUS_NULL					"\0\0"
#define	AIM_STATUS_ONLINE				"\0\0"
#define	AIM_STATUS_AWAY					"\0\x01"
#define	AIM_STATUS_DND					"\0\x02"
#define	AIM_STATUS_NA					"\0\x04"
#define	AIM_STATUS_OCCUPIED				"\0\x10"
#define	AIM_STATUS_FREE4CHAT			"\0\x20"
#define AIM_STATUS_INVISIBLE			"\x01\0"
#endif
