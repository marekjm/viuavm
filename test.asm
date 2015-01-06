istore 1 16
istore 2 1
;name 1 sixteen
; or...
;.name: 1 sixteen
; so whether do names as real CPU instructions or
; as assembler constructs, kinda like .mark: and
; resolve them at the asm level
;
; it would feel OK but, huh, Saint-Excupery said that
; the best engineering is when there is nothing to remove and
; after all name as CPU can be quite more-than-needed
;
; let's do this as asm constructs for now and
; worry about how to do struct's later, as I feel it's not
; teh right direction anyway...
;
; I need to read more
;
; TODO: move this to developer diary as a 2015.01.07 entry
.name: 1 sixteen
ilt 2 1 3
branchif 3 5 4
print 3
branch 6
istore 4 @1
istore 5 4
iinc @5
iinc 4
print @5
halt
