try {
	throw function(e) {return ;}
} catch(e) {
	print(e(e))
}