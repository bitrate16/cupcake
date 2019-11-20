namespace ck_token {
	// Pre-defined type codes
	const int INTEGER	     = 12; // int
	const int DOUBLE         = 13; // double
	const int BOOLEAN        = 14; // bool
	const int STRING         = 15; // string
	const int NAME           = 16; // name/key
	const int TNULL          = 17; // null
	const int UNDEFINED      = 18; // undefined
	const int ARRAY          = 19; // array
	const int OBJECT         = 20; // object

	// Cupcake keywords
	const int IF             = 47;
	const int ELSE           = 48;
	const int SWITCH         = 49;
	const int CASE           = 50;
	const int DEFAULT        = 51;
	const int BREAK          = 52;
	const int CONTINUE       = 53;
	const int RETURN         = 54;
	const int WHILE          = 55;
	const int DO             = 56;
	const int FOR            = 57;
	const int IN             = 58;
	const int FUNCTION       = 59;
	const int VAR            = 60;
	const int TRY            = 61;
	const int CATCH          = 62;
	const int THROW          = 63;
	const int CONST          = 64;
	const int SAFE           = 65;
	const int LOCAL          = 66;
	const int TRUE           = 67;
	const int FALSE          = 68;
	const int SELF           = 69;
	const int THIS           = 71;
	const int WITH           = 72;
	const int TYPEOF         = 73;
	const int ISTYPEOF       = 74;
	const int AS             = 75;
	
	// Cupcake operators
	const int ASSIGN         =  90; // =
	const int HOOK           =  91; // ?
	const int COLON          =  92; // :
	const int DOT            =  93; // .
	const int COMMA          =  94; // ,
	const int SEMICOLON      =  95; // ;
	const int LP             =  96; // (
	const int RP             =  97; // )
	const int LB             =  98; // [
	const int RB             =  99; // ]
	const int LC             = 100; // {
	const int RC             = 101; // }
	const int EQ             = 102; // ==
	const int NEQ            = 103; // !=
	const int LEQ            = 104; // ===
	const int NLEQ           = 105; // !==
	const int OR             = 106; // ||
	const int AND            = 107; // &
	const int BITOR          = 108; // |
	const int BITAND         = 109; // &
	const int BITXOR         = 110; // ^
	const int GT             = 111; // >
	const int GE             = 112; // >=
	const int LT             = 113; // <
	const int LE             = 114; // <=
	const int BITRSH         = 115; // <<
	const int BITLSH         = 116; // >>
	const int BITURSH        = 117; // >>>
	const int BITULSH        = 118; // <<<
	const int PLUS           = 119; // +
	const int MINUS          = 120; // -
	const int MUL            = 121; // *
	const int DIV            = 122; // /
	const int DIR            = 123; // \\ .
	const int MOD            = 124; // %
	const int HASH           = 125; // #
	const int NOT            = 126; // !
	const int BITNOT         = 127; // ~
	const int INC            = 128; // ++
	const int DEC            = 129; // --
	const int ASSIGN_ADD     = 130; // +=
	const int ASSIGN_SUB     = 131; // -=
	const int ASSIGN_MUL     = 132; // *=
	const int ASSIGN_DIV     = 133; // /=
	const int ASSIGN_BITRSH  = 134; // >>=
	const int ASSIGN_BITLSH  = 135; // <<=
	const int ASSIGN_BITURSH = 136; // >>>=
	const int ASSIGN_BITULSH = 137; // <<<=
	const int ASSIGN_DIR     = 138; // \\=
	const int ASSIGN_PATH    = 139; // \=
	const int ASSIGN_MOD     = 140; // %=
	const int ASSIGN_BITOR   = 141; // |=
	const int ASSIGN_BITAND  = 142; // &=
	const int ASSIGN_BITXOR  = 143; // ^=
	const int PATH           = 144; // \ .
	const int PUSH           = 145; // ->
	const int ARROW          = 146; // =>
	const int DOG            = 147; // @x
	const int ASSIGN_HASH    = 148; // #=
	                          
	const int PRE_INC        = 160; // ++x
	const int PRE_DEC        = 154; // --x
	const int POS_INC        = 161; // x++
	const int POS_DEC        = 162; // x--
	const int POS            = 163; // +x
	const int NEG            = 164; // -x

	// Technical tokens
	const int TEOF          = WEOF;
	const int TEOL          = U'\n';
	const int TERR          = 213;
	const int NONE          = 214;

	// Additional statements & expressions
	const int FIELD           = 303; // a.field
	const int MEMBER          = 304; // a[<expression>]
	const int CALL            = 305; // func(...)
	const int EMPTY           = 306; // ...
	const int BLOCK           = 307; // { ... }
	const int DEFINE          = 308; // var a = ...
	
	const int EXPRESSION      = 309;
	const int ASTROOT         = 310;
	const int CONDITION       = 311; // ?:
};

