;@78           56           34           12         
;@78 56        56 34        34 12        12 00      
;@78 56 34     56 34 12     34 12 00     12 00 00   
;@78 56 34 12  56 34 12 00  34 12 00 00  12 00 00 00
;@
;@78           56           34           12         
;@78 56        56 34        34 12        12 00      
;@78 56 34     56 34 12     34 12 00     12 00 00   
;@78 56 34 12  56 34 12 00  34 12 00 00  12 00 00 00
;@
;@78           56           34           12         
;@78 56        56 34        34 12        12 00      
;@78 56 34     56 34 12     34 12 00     12 00 00   
;@78 56 34 12  56 34 12 00  34 12 00 00  12 00 00 00

org $008000

table table.tbl
db "ABCD"
dw "ABCD"
dl "ABCD"
dd "ABCD"

table table.tbl,ltr
db "ABCD"
dw "ABCD"
dl "ABCD"
dd "ABCD"

table table-rtl.tbl,rtl
db "ABCD"
dw "ABCD"
dl "ABCD"
dd "ABCD"