// Code generated by gocc; DO NOT EDIT.

package lexer

import (
	"io/ioutil"
	"unicode/utf8"

	"mlr/src/parsing/token"
)

const (
	NoState    = -1
	NumStates  = 320
	NumSymbols = 550
)

type Lexer struct {
	src     []byte
	pos     int
	line    int
	column  int
	Context token.Context
}

func NewLexer(src []byte) *Lexer {
	lexer := &Lexer{
		src:     src,
		pos:     0,
		line:    1,
		column:  1,
		Context: nil,
	}
	return lexer
}

// SourceContext is a simple instance of a token.Context which
// contains the name of the source file.
type SourceContext struct {
	Filepath string
}

func (s *SourceContext) Source() string {
	return s.Filepath
}

func NewLexerFile(fpath string) (*Lexer, error) {
	src, err := ioutil.ReadFile(fpath)
	if err != nil {
		return nil, err
	}
	lexer := NewLexer(src)
	lexer.Context = &SourceContext{Filepath: fpath}
	return lexer, nil
}

func (l *Lexer) Scan() (tok *token.Token) {
	tok = &token.Token{}
	if l.pos >= len(l.src) {
		tok.Type = token.EOF
		tok.Pos.Offset, tok.Pos.Line, tok.Pos.Column = l.pos, l.line, l.column
		tok.Pos.Context = l.Context
		return
	}
	start, startLine, startColumn, end := l.pos, l.line, l.column, 0
	tok.Type = token.INVALID
	state, rune1, size := 0, rune(-1), 0
	for state != -1 {
		if l.pos >= len(l.src) {
			rune1 = -1
		} else {
			rune1, size = utf8.DecodeRune(l.src[l.pos:])
			l.pos += size
		}

		nextState := -1
		if rune1 != -1 {
			nextState = TransTab[state](rune1)
		}
		state = nextState

		if state != -1 {

			switch rune1 {
			case '\n':
				l.line++
				l.column = 1
			case '\r':
				l.column = 1
			case '\t':
				l.column += 4
			default:
				l.column++
			}

			switch {
			case ActTab[state].Accept != -1:
				tok.Type = ActTab[state].Accept
				end = l.pos
			case ActTab[state].Ignore != "":
				start, startLine, startColumn = l.pos, l.line, l.column
				state = 0
				if start >= len(l.src) {
					tok.Type = token.EOF
				}

			}
		} else {
			if tok.Type == token.INVALID {
				end = l.pos
			}
		}
	}
	if end > start {
		l.pos = end
		tok.Lit = l.src[start:end]
	} else {
		tok.Lit = []byte{}
	}
	tok.Pos.Offset, tok.Pos.Line, tok.Pos.Column = start, startLine, startColumn
	tok.Pos.Context = l.Context

	return
}

func (l *Lexer) Reset() {
	l.pos = 0
}

/*
Lexer symbols:
0: '"'
1: '"'
2: '"'
3: '"'
4: 'i'
5: '0'
6: 'x'
7: '0'
8: 'b'
9: '.'
10: '.'
11: '-'
12: '.'
13: '.'
14: '-'
15: '.'
16: '.'
17: '-'
18: 'M'
19: '_'
20: 'P'
21: 'I'
22: 'M'
23: '_'
24: 'E'
25: 'n'
26: 'u'
27: 'l'
28: 'l'
29: 'I'
30: 'P'
31: 'S'
32: 'I'
33: 'F'
34: 'S'
35: 'I'
36: 'R'
37: 'S'
38: 'O'
39: 'P'
40: 'S'
41: 'O'
42: 'F'
43: 'S'
44: 'O'
45: 'R'
46: 'S'
47: 'F'
48: 'L'
49: 'A'
50: 'T'
51: 'S'
52: 'E'
53: 'P'
54: 'N'
55: 'F'
56: 'N'
57: 'R'
58: 'F'
59: 'N'
60: 'R'
61: 'F'
62: 'I'
63: 'L'
64: 'E'
65: 'N'
66: 'A'
67: 'M'
68: 'E'
69: 'F'
70: 'I'
71: 'L'
72: 'E'
73: 'N'
74: 'U'
75: 'M'
76: 'E'
77: 'N'
78: 'V'
79: 'b'
80: 'e'
81: 'g'
82: 'i'
83: 'n'
84: 'd'
85: 'o'
86: 'e'
87: 'l'
88: 'i'
89: 'f'
90: 'e'
91: 'l'
92: 's'
93: 'e'
94: 'e'
95: 'n'
96: 'd'
97: 'f'
98: 'i'
99: 'l'
100: 't'
101: 'e'
102: 'r'
103: 'f'
104: 'o'
105: 'r'
106: 'i'
107: 'f'
108: 'i'
109: 'n'
110: 'w'
111: 'h'
112: 'i'
113: 'l'
114: 'e'
115: 'b'
116: 'r'
117: 'e'
118: 'a'
119: 'k'
120: 'c'
121: 'o'
122: 'n'
123: 't'
124: 'i'
125: 'n'
126: 'u'
127: 'e'
128: 'r'
129: 'e'
130: 't'
131: 'u'
132: 'r'
133: 'n'
134: 'f'
135: 'u'
136: 'n'
137: 'c'
138: 's'
139: 'u'
140: 'b'
141: 'r'
142: 'c'
143: 'a'
144: 'l'
145: 'l'
146: 'a'
147: 'r'
148: 'r'
149: 'b'
150: 'o'
151: 'o'
152: 'l'
153: 'f'
154: 'l'
155: 'o'
156: 'a'
157: 't'
158: 'i'
159: 'n'
160: 't'
161: 'm'
162: 'a'
163: 'p'
164: 'n'
165: 'u'
166: 'm'
167: 's'
168: 't'
169: 'r'
170: 'v'
171: 'a'
172: 'r'
173: 'u'
174: 'n'
175: 's'
176: 'e'
177: 't'
178: 'd'
179: 'u'
180: 'm'
181: 'p'
182: 'e'
183: 'd'
184: 'u'
185: 'm'
186: 'p'
187: 'e'
188: 'm'
189: 'i'
190: 't'
191: 'e'
192: 'm'
193: 'i'
194: 't'
195: 'p'
196: 'e'
197: 'm'
198: 'i'
199: 't'
200: 'f'
201: 'e'
202: 'p'
203: 'r'
204: 'i'
205: 'n'
206: 't'
207: 'e'
208: 'p'
209: 'r'
210: 'i'
211: 'n'
212: 't'
213: 'n'
214: 'p'
215: 'r'
216: 'i'
217: 'n'
218: 't'
219: 'p'
220: 'r'
221: 'i'
222: 'n'
223: 't'
224: 'n'
225: 't'
226: 'e'
227: 'e'
228: 's'
229: 't'
230: 'd'
231: 'o'
232: 'u'
233: 't'
234: 's'
235: 't'
236: 'd'
237: 'e'
238: 'r'
239: 'r'
240: '$'
241: '$'
242: '{'
243: '}'
244: '$'
245: '*'
246: '@'
247: '@'
248: '{'
249: '}'
250: '@'
251: '*'
252: 'a'
253: 'l'
254: 'l'
255: '%'
256: '%'
257: '%'
258: 'p'
259: 'a'
260: 'n'
261: 'i'
262: 'c'
263: '%'
264: '%'
265: '%'
266: ';'
267: '{'
268: '}'
269: '='
270: '>'
271: '>'
272: '>'
273: '|'
274: ','
275: '('
276: ')'
277: '$'
278: '['
279: ']'
280: '$'
281: '['
282: '['
283: '$'
284: '['
285: '['
286: '['
287: '@'
288: '['
289: '|'
290: '|'
291: '='
292: '^'
293: '^'
294: '='
295: '&'
296: '&'
297: '='
298: '?'
299: '?'
300: '='
301: '?'
302: '?'
303: '?'
304: '='
305: '|'
306: '='
307: '&'
308: '='
309: '^'
310: '='
311: '<'
312: '<'
313: '='
314: '>'
315: '>'
316: '='
317: '>'
318: '>'
319: '>'
320: '='
321: '+'
322: '='
323: '.'
324: '='
325: '-'
326: '='
327: '*'
328: '='
329: '/'
330: '='
331: '/'
332: '/'
333: '='
334: '%'
335: '='
336: '*'
337: '*'
338: '='
339: '?'
340: ':'
341: '|'
342: '|'
343: '^'
344: '^'
345: '&'
346: '&'
347: '?'
348: '?'
349: '?'
350: '?'
351: '?'
352: '='
353: '~'
354: '!'
355: '='
356: '~'
357: '='
358: '='
359: '!'
360: '='
361: '<'
362: '='
363: '>'
364: '>'
365: '='
366: '<'
367: '<'
368: '='
369: '^'
370: '&'
371: '<'
372: '<'
373: '>'
374: '>'
375: '>'
376: '+'
377: '-'
378: '.'
379: '+'
380: '.'
381: '-'
382: '*'
383: '/'
384: '/'
385: '/'
386: '%'
387: '.'
388: '*'
389: '.'
390: '/'
391: '.'
392: '/'
393: '/'
394: '.'
395: '!'
396: '~'
397: '*'
398: '*'
399: '['
400: '['
401: '['
402: '['
403: '['
404: '['
405: '_'
406: '_'
407: ' '
408: '!'
409: '#'
410: '$'
411: '%'
412: '&'
413: '''
414: '\'
415: '('
416: ')'
417: '*'
418: '+'
419: ','
420: '-'
421: '.'
422: '/'
423: ':'
424: ';'
425: '<'
426: '='
427: '>'
428: '?'
429: '@'
430: '['
431: ']'
432: '^'
433: '_'
434: '`'
435: '{'
436: '|'
437: '}'
438: '~'
439: '\'
440: '\'
441: '\'
442: '"'
443: '\'
444: '['
445: '\'
446: ']'
447: '\'
448: 'b'
449: '\'
450: 'f'
451: '\'
452: 'n'
453: '\'
454: 'r'
455: '\'
456: 't'
457: '\'
458: 'x'
459: '\'
460: '.'
461: '\'
462: '*'
463: '\'
464: '0'
465: '\'
466: '1'
467: '\'
468: '2'
469: '\'
470: '3'
471: '\'
472: '4'
473: '\'
474: '5'
475: '\'
476: '6'
477: '\'
478: '7'
479: '\'
480: '8'
481: '\'
482: '9'
483: 'e'
484: 'E'
485: 't'
486: 'r'
487: 'u'
488: 'e'
489: 'f'
490: 'a'
491: 'l'
492: 's'
493: 'e'
494: ' '
495: '!'
496: '#'
497: '$'
498: '%'
499: '&'
500: '''
501: '\'
502: '('
503: ')'
504: '*'
505: '+'
506: ','
507: '-'
508: '.'
509: '/'
510: ':'
511: ';'
512: '<'
513: '='
514: '>'
515: '?'
516: '@'
517: '['
518: ']'
519: '^'
520: '_'
521: '`'
522: '|'
523: '~'
524: '\'
525: '{'
526: '\'
527: '}'
528: ' '
529: '\t'
530: '\n'
531: '\r'
532: '#'
533: '\n'
534: 'a'-'z'
535: 'A'-'Z'
536: '0'-'9'
537: '0'-'9'
538: 'a'-'f'
539: 'A'-'F'
540: '0'-'1'
541: 'A'-'Z'
542: 'a'-'z'
543: '0'-'9'
544: \u0100-\U0010ffff
545: 'A'-'Z'
546: 'a'-'z'
547: '0'-'9'
548: \u0100-\U0010ffff
549: .
*/
