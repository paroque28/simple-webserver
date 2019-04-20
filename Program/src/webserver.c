#include "webserver.h"


types_t extensions[] = { 
    //Definition of the supported extensions
	{"gif", "image/gif" },  
	{"jpg", "image/jpeg"}, 
	{"jpeg","image/jpeg"},
	{"png", "image/png" },  
	{"zip", "image/zip" },  
	{"gz",  "image/gz"  },  
	{"tar", "image/tar" },  
	{"htm", "text/html" },  
	{"html","text/html" },  
	{"php", "text/php"  },  
	{"cgi", "text/cgi"  },  
	{"asp","text/asp"   },  
	{"jsp", "image/jsp" },  
	{"xml", "text/xml"  },  
	{"js","text/js"     },
    {"css","test/css"   }, 
	{"ico","image/ico"  },
	{"iso","image/iso"  },
	{"txt","text/txt"   },
	{0,0} };

