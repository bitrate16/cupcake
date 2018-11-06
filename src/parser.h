
namespace ck_token {
	// Pre-defined type codes
	const int INTEGER	                 12;
	const int DOUBLE                     13;
	const int BOOLEAN                    14;
	const int STRING                     15;
	const int NAME                       16;
	const int FLAG                       17;
	const int LONG                       18;
	const int BYTE                       19;
	const int ARRAY                      20;
	const int OBJECT                     21;
	const int SCOPE                      22;
	const int STRING_PROTOTYPE           23;
	const int DOUBLE_PROTOTYPE           24;
	const int NULL_PROTOTYPE             25;
	const int OBJECT_PROTOTYPE           26;
	const int SCOPE_PROTOTYPE            27;
	const int PROXY_SCOPE                28;
	const int CODE_FUNCTION_PROTOTYPE    29;
	const int CODE_FUNCTION              30;
	const int NATIVE_FUNCTION_PROTOTYPE  31;
	const int NATIVE_FUNCTION            32;
	const int INTEGER_PROTOTYPE          33;
	const int BOOLEAN_PROTOTYPE          34;
	const int UNDEFINED_PROTOTYPE        35;
	const int TNULL                      36;
	const int UNDEFINED                  37;
	const int ARRAY_PROTOTYPE            38;
	const int ERROR                      39;
	const int ERROR_PROTOTYPE            40;

	// Cupcake keywords
	const int IF               47;
	const int ELSE             48;
	const int SWITCH           49;
	const int CASE             50;
	const int DEFAULT          74;
	const int BREAK            51;
	const int CONTINUE         52;
	const int RETURN           53;
	const int WHILE            54;
	const int DO               55;
	const int FOR              56;
	const int IN               57;
	const int FUNCTION         58;
	const int VAR              59;
	const int TRY              60;
	const int EXPECT           61;
	const int RAISE            62;
	const int PROTOTYPE        63;
	const int CONST            64;
	const int SAFE             65;
	const int NEW              66;
	const int LOCAL            67;
	const int TRUE             68;
	const int FALSE            69;
	const int SELF             72;
	const int THIS             73;
	
	// Cupcake operators
	const int ASSIGN           80;
	const int HOOK             81;
	const int COLON            82;
	const int DOT              83;
	const int COMMA            84;
	const int SEMICOLON        85;
	const int LP               86;
	const int RP               87;
	const int LB               88;
	const int RB               89;
	const int LC               90;
	const int RC               91;
	const int EQ               92;
	const int NEQ              93;
	const int OR               94;
	const int AND              95;
	const int BITOR            96;
	const int BITAND           97;
	const int BITXOR           98;
	const int GT               99;
	const int GE              100;
	const int LT              101;
	const int LE              102;
	const int BITRSH          103;
	const int BITLSH          104;
	const int BITURSH         105;
	const int PLUS            106;
	const int MINUS           107;
	const int MUL             108;
	const int DIV             109;
	const int MDIV            110;
	const int MOD             111;
	const int HASH            112;
	const int NOT             113;
	const int BITNOT          114;
	const int INC             115;
	const int DEC             116;
	const int ASSIGN_ADD      117;
	const int ASSIGN_SUB      118;
	const int ASSIGN_MUL      119;
	const int ASSIGN_DIV      120;
	const int ASSIGN_BITRSH   121;
	const int ASSIGN_BITLSH   122;
	const int ASSIGN_BITURSH  123;
	const int ASSIGN_BITNOT   124;
	const int ASSIGN_MDIV     125;
	const int ASSIGN_MOD      126;
	const int ASSIGN_BITOR    127;
	const int ASSIGN_BITAND   128;
	const int ASSIGN_BITXOR   129;
	const int PATH            130;
	const int PUSH            131;
	const int LAMBDA          132;
	const int PRE_INC         133;
	const int PRE_DEC         134;
	const int POS_INC         135;
	const int POS_DEC         136;
	const int POS             157;
	const int NEG             158;

	// Technical tokens
	const int TEOF            137;
	const int TEOL            '\n';
	const int TERR            139;
	const int NONE            140;

	// Additional statements & expressions
	const int EMPTY           201;
	const int EXPRESSION      202;
	const int BLOCK           203;
	const int ASSIGN_DEFINE   204;
	const int DEFINE          205;
	const int ASTROOT         206;
	const int CONDITION       207;
	const int MEMBER          208;
	const int FIELD           209;
	const int CALL            210;
	const int NATIVE_CALL     211;
	const int IMPORTED_SCRIPT 212;
	const int FUNCTIONROOT    213;
};

namespace ck_parse {
	class raw_token {
		int token;
		int integerv;
		long longv;
		int bytev;
		bool booleanv;
		double doublev;
		std::string stringv
		int lineno;
		
		~raw_token();
		
		raw_token *copy();
	};
	
	class tokenizer {
		
	};
	
	class parser {
		
	};
};