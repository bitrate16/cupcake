#include "parser.h"

int main() {
	FILE *f = fopen("test.ck", "w");
	
	if (!f)
		return 1;
	
	parser_messages pm;
	parser(pm);
	
	stream_wrapper sw(f);
	
	parser.set_stream(sw);
};