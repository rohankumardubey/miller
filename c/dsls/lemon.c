/*
** This file contains all sources (including headers) to the LEMON
** LALR(1) parser generator.  The sources have been combined into a
** single file to make it easy to include LEMON in the source tree
** and Makefile of another program.
**
** The author of this program disclaims copyright.
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>

#include "lemon_dims.h"
#include "lemon_assert.h"
#include "lemon_error.h"
#include "lemon_memory.h"
#include "lemon_option.h"
#include "lemon_structs.h"
#include "lemon_action.h"
#include "lemon_string.h"
#include "lemon_report.h"
#include "lemon_symbol.h"
#include "lemon_plink.h"
#include "lemon_parse.h"

char *msort();

/********** From the file "build.h" ************************************/
void FindRulePrecedences();
void FindFirstSets();
void FindStates();
void FindLinks();
void FindFollowSets();
void FindActions();

/********* From the file "configlist.h" *********************************/
void Configlist_init(/* void */);
struct config *Configlist_add(/* struct rule *, int */);
struct config *Configlist_addbasis(/* struct rule *, int */);
void Configlist_closure(/* void */);
void Configlist_sort(/* void */);
void Configlist_sortbasis(/* void */);
struct config *Configlist_return(/* void */);
struct config *Configlist_basis(/* void */);
void Configlist_eat(/* struct config * */);
void Configlist_reset(/* void */);

/********** From the file "set.h" ****************************************/
void  SetSize(/* int N */);             /* All sets will be of size N */
char *SetNew(/* void */);               /* A new set for element 0..N */
void  SetFree(/* char* */);             /* Deallocate a set */

int SetAdd(/* char*,int */);            /* Add element to a set */
int SetUnion(/* char *A,char *B */);    /* A <- A U B, thru element N */

#define SetFind(X,Y) (X[Y])       /* True if Y is in set X */

/********** From the file "struct.h" *************************************/
/*
** Principal data structures for the LEMON parser generator.
*/

/**************** From the file "table.h" *********************************/
/*
** All code in this file has been automatically generated
** from a specification in the file
**              "table.q"
** by the associative array code building program "aagen".
** Do not edit this file!  Instead, edit the specification
** file, then rerun aagen.
*/
/*
** Code for processing tables in the LEMON parser generator.
*/

/* Routines for handling symbols of the grammar */

struct symbol *Symbol_new();
int Symbolcmpp(/* struct symbol **, struct symbol ** */);
void Symbol_init(/* void */);
int Symbol_insert(/* struct symbol *, char * */);
struct symbol *Symbol_find(/* char * */);
struct symbol *Symbol_Nth(/* int */);
int Symbol_count(/*  */);
struct symbol **Symbol_arrayof(/*  */);

/* Routines to manage the state table */

int Configcmp(/* struct config *, struct config * */);
struct state *State_new();
void State_init(/* void */);
int State_insert(/* struct state *, struct config * */);
struct state *State_find(/* struct config * */);
struct state **State_arrayof(/*  */);

/* Routines used for efficiency in Configlist_add */

void Configtable_init(/* void */);
int Configtable_insert(/* struct config * */);
struct config *Configtable_find(/* struct config * */);
void Configtable_clear(/* int(*)(struct config *) */);

/********************** From the file "build.c" *****************************/
/*
** Routines to construction the finite state machine for the LEMON
** parser generator.
*/

/* Find a precedence symbol of every rule in the grammar.
**
** Those rules which have a precedence symbol coded in the input
** grammar using the "[symbol]" construct will already have the
** rp->precsym field filled.  Other rules take as their precedence
** symbol the first RHS symbol with a defined precedence.  If there
** are not RHS symbols with a defined precedence, the precedence
** symbol field is left blank.
*/
void FindRulePrecedences(struct lemon *xp)
{
	struct rule *rp;
	for(rp=xp->rule; rp; rp=rp->next){
		if (rp->precsym==0) {
			int i;
			for(i=0; i<rp->nrhs; i++){
				if (rp->rhs[i]->prec>=0) {
					rp->precsym = rp->rhs[i];
					break;
	}
			}
		}
	}
	return;
}

/* Find all nonterminals which will generate the empty string.
** Then go back and compute the first sets of every nonterminal.
** The first set is the set of all terminal symbols which can begin
** a string generated by that nonterminal.
*/
void FindFirstSets(struct lemon *lemp)
{
	int i;
	struct rule *rp;
	int progress;

	for (i=0; i<lemp->nsymbol; i++) {
		lemp->symbols[i]->lambda = B_FALSE;
	}
	for (i=lemp->nterminal; i<lemp->nsymbol; i++) {
		lemp->symbols[i]->firstset = SetNew();
	}

	/* First compute all lambdas */
	do {
		progress = 0;
		for (rp=lemp->rule; rp; rp=rp->next) {
			if (rp->lhs->lambda)  continue;
			for(i=0; i<rp->nrhs; i++){
				 if (rp->rhs[i]->lambda==B_FALSE)  break;
			}
			if (i==rp->nrhs) {
				rp->lhs->lambda = B_TRUE;
				progress = 1;
			}
		}
	} while (progress);

	/* Now compute all first sets */
	do {
		struct symbol *s1, *s2;
		progress = 0;
		for(rp=lemp->rule; rp; rp=rp->next){
			s1 = rp->lhs;
			for(i=0; i<rp->nrhs; i++){
				s2 = rp->rhs[i];
				if (s2->type==TERMINAL) {
					progress += SetAdd(s1->firstset,s2->index);
					break;
				} else if (s1==s2) {
					if (s1->lambda==B_FALSE)  break;
				} else {
					progress += SetUnion(s1->firstset,s2->firstset);
					if (s2->lambda==B_FALSE)  break;
				}
			}
		}
	} while (progress);
	return;
}

/* Compute all LR(0) states for the grammar.  Links
** are added to between some states so that the LR(1) follow sets
** can be computed later.
*/
static struct state *getstate(/* struct lemon * */);  /* forward reference */

void FindStates(struct lemon *lemp)
{
	struct symbol *sp;
	struct rule *rp;

	Configlist_init();

	/* Find the start symbol */
	if (lemp->start) {
		sp = Symbol_find(lemp->start);
		if (sp==0) {
			ErrorMsg(lemp->filename,0,
				"The specified start symbol \"%s\" is not \
				in a nonterminal of the grammar.  \"%s\" will be used as the start \
				symbol instead.",lemp->start,lemp->rule->lhs->name);
			lemp->errorcnt++;
			sp = lemp->rule->lhs;
		}
	} else {
		sp = lemp->rule->lhs;
	}

	/* Make sure the start symbol doesn't occur on the right-hand side of
	** any rule.  Report an error if it does.  (YACC would generate a new
	** start symbol in this case.) */
	for(rp=lemp->rule; rp; rp=rp->next){
		int i;
		for(i=0; i<rp->nrhs; i++){
			if (rp->rhs[i]==sp) {
				ErrorMsg(lemp->filename,0,
					"The start symbol \"%s\" occurs on the \
					right-hand side of a rule. This will result in a parser which \
					does not work properly.",sp->name);
				lemp->errorcnt++;
			}
		}
	}

	/* The basis configuration set for the first state
	** is all rules which have the start symbol as their
	** left-hand side */
	for(rp=sp->rule; rp; rp=rp->nextlhs){
		struct config *newcfp;
		newcfp = Configlist_addbasis(rp,0);
		SetAdd(newcfp->fws,0);
	}

	/* Compute the first state.  All other states will be
	** computed automatically during the computation of the first one.
	** The returned pointer to the first state is not used. */
	(void)getstate(lemp);
	return;
}

/* Return a pointer to a state which is described by the configuration
** list which has been built from calls to Configlist_add.
*/
static void buildshifts(/* struct lemon *, struct state * */); /* Forward ref */

static struct state *getstate(struct lemon *lemp)
{
	struct config *cfp, *bp;
	struct state *stp;

	/* Extract the sorted basis of the new state.  The basis was constructed
	** by prior calls to "Configlist_addbasis()". */
	Configlist_sortbasis();
	bp = Configlist_basis();

	/* Get a state with the same basis */
	stp = State_find(bp);
	if (stp) {
		/* A state with the same basis already exists!  Copy all the follow-set
		** propagation links from the state under construction into the
		** preexisting state, then return a pointer to the preexisting state */
		struct config *x, *y;
		for(x=bp, y=stp->bp; x && y; x=x->bp, y=y->bp){
			Plink_copy(&y->bplp,x->bplp);
			Plink_delete(x->fplp);
			x->fplp = x->bplp = 0;
		}
		cfp = Configlist_return();
		Configlist_eat(cfp);
	} else {
		/* This really is a new state.  Construct all the details */
		Configlist_closure(lemp);    /* Compute the configuration closure */
		Configlist_sort();           /* Sort the configuration closure */
		cfp = Configlist_return();   /* Get a pointer to the config list */
		stp = State_new();           /* A new state structure */
		MemoryCheck(stp);
		stp->bp = bp;                /* Remember the configuration basis */
		stp->cfp = cfp;              /* Remember the configuration closure */
		stp->index = lemp->nstate++; /* Every state gets a sequence number */
		stp->ap = 0;                 /* No actions, yet. */
		State_insert(stp,stp->bp);   /* Add to the state table */
		buildshifts(lemp,stp);       /* Recursively compute successor states */
	}
	return stp;
}

/* Construct all successor states to the given state.  A "successor"
** state is any state which can be reached by a shift action.
*/
static void buildshifts(
	struct lemon *lemp,
	struct state *stp)     /* The state from which successors are computed */
{
	struct config *cfp;  /* For looping thru the config closure of "stp" */
	struct config *bcfp; /* For the inner loop on config closure of "stp" */
	struct config *new;  /* */
	struct symbol *sp;   /* Symbol following the dot in configuration "cfp" */
	struct symbol *bsp;  /* Symbol following the dot in configuration "bcfp" */
	struct state *newstp; /* A pointer to a successor state */

	/* Each configuration becomes complete after it contibutes to a successor
	** state.  Initially, all configurations are incomplete */
	for(cfp=stp->cfp; cfp; cfp=cfp->next) cfp->status = INCOMPLETE;

	/* Loop through all configurations of the state "stp" */
	for(cfp=stp->cfp; cfp; cfp=cfp->next){
		if (cfp->status==COMPLETE)  continue;    /* Already used by inner loop */
		if (cfp->dot>=cfp->rp->nrhs)  continue;  /* Can't shift this config */
		Configlist_reset();                      /* Reset the new config set */
		sp = cfp->rp->rhs[cfp->dot];             /* Symbol after the dot */

		/* For every configuration in the state "stp" which has the symbol "sp"
		** following its dot, add the same configuration to the basis set under
		** construction but with the dot shifted one symbol to the right. */
		for(bcfp=cfp; bcfp; bcfp=bcfp->next){
			if (bcfp->status==COMPLETE)  continue;    /* Already used */
			if (bcfp->dot>=bcfp->rp->nrhs)  continue; /* Can't shift this one */
			bsp = bcfp->rp->rhs[bcfp->dot];           /* Get symbol after dot */
			if (bsp!=sp)  continue;                   /* Must be same as for "cfp" */
			bcfp->status = COMPLETE;                  /* Mark this config as used */
			new = Configlist_addbasis(bcfp->rp,bcfp->dot+1);
			Plink_add(&new->bplp,bcfp);
		}

		/* Get a pointer to the state described by the basis configuration set
		** constructed in the preceding loop */
		newstp = getstate(lemp);

		/* The state "newstp" is reached from the state "stp" by a shift action
		** on the symbol "sp" */
		Action_add(&stp->ap,SHIFT,sp,(char *)newstp);
	}
}

/*
** Construct the propagation links
*/
void FindLinks(struct lemon *lemp)
{
	int i;
	struct config *cfp, *other;
	struct state *stp;
	struct plink *plp;

	/* Housekeeping detail:
	** Add to every propagate link a pointer back to the state to
	** which the link is attached. */
	for(i=0; i<lemp->nstate; i++){
		stp = lemp->sorted[i];
		for(cfp=stp->cfp; cfp; cfp=cfp->next){
			cfp->stp = stp;
		}
	}

	/* Convert all backlinks into forward links.  Only the forward
	** links are used in the follow-set computation. */
	for(i=0; i<lemp->nstate; i++){
		stp = lemp->sorted[i];
		for(cfp=stp->cfp; cfp; cfp=cfp->next){
			for(plp=cfp->bplp; plp; plp=plp->next){
				other = plp->cfp;
				Plink_add(&other->fplp,cfp);
			}
		}
	}
}

/* Compute all followsets.
**
** A followset is the set of all symbols which can come immediately
** after a configuration.
*/
void FindFollowSets(struct lemon *lemp)
{
	int i;
	struct config *cfp;
	struct plink *plp;
	int progress;
	int change;

	for(i=0; i<lemp->nstate; i++){
		for(cfp=lemp->sorted[i]->cfp; cfp; cfp=cfp->next){
			cfp->status = INCOMPLETE;
		}
	}

	do {
		progress = 0;
		for (i=0; i<lemp->nstate; i++){
			for (cfp=lemp->sorted[i]->cfp; cfp; cfp=cfp->next) {
				if (cfp->status==COMPLETE)  continue;
				for (plp=cfp->fplp; plp; plp=plp->next) {
					change = SetUnion(plp->cfp->fws,cfp->fws);
					if (change) {
						plp->cfp->status = INCOMPLETE;
						progress = 1;
					}
				}
				cfp->status = COMPLETE;
			}
		}
	} while (progress);
}

static int resolve_conflict();

/* Compute the reduce actions, and resolve conflicts.
*/
void FindActions(struct lemon *lemp)
{
	int i,j;
	struct config *cfp;
	struct state *stp;
	struct symbol *sp;
	struct rule *rp;

	/* Add all of the reduce actions
	** A reduce action is added for each element of the followset of
	** a configuration which has its dot at the extreme right.
	*/
	for(i=0; i<lemp->nstate; i++){   /* Loop over all states */
		stp = lemp->sorted[i];
		for(cfp=stp->cfp; cfp; cfp=cfp->next){  /* Loop over all configurations */
			if (cfp->rp->nrhs==cfp->dot) {        /* Is dot at extreme right? */
				for(j=0; j<lemp->nterminal; j++){
					if (SetFind(cfp->fws,j)) {
						/* Add a reduce action to the state "stp" which will reduce by the
						** rule "cfp->rp" if the lookahead symbol is "lemp->symbols[j]" */
						Action_add(&stp->ap,REDUCE,lemp->symbols[j],(char *)cfp->rp);
					}
				}
			}
		}
	}

	/* Add the accepting token */
	if (lemp->start) {
		sp = Symbol_find(lemp->start);
		if (sp==0)  sp = lemp->rule->lhs;
	} else {
		sp = lemp->rule->lhs;
	}
	/* Add to the first state (which is always the starting state of the
	** finite state machine) an action to ACCEPT if the lookahead is the
	** start nonterminal.  */
	Action_add(&lemp->sorted[0]->ap,ACCEPT,sp,0);

	/* Resolve conflicts */
	for(i=0; i<lemp->nstate; i++){
		struct action *ap, *nap;
		struct state *stp;
		stp = lemp->sorted[i];
		assert (stp->ap) ;
		stp->ap = Action_sort(stp->ap);
		for(ap=stp->ap; ap && ap->next; ap=ap->next){
			for(nap=ap->next; nap && nap->sp==ap->sp; nap=nap->next){
				 /* The two actions "ap" and "nap" have the same lookahead.
				 ** Figure out which one should be used */
				 lemp->nconflict += resolve_conflict(ap,nap,lemp->errsym);
			}
		}
	}

	/* Report an error for each rule that can never be reduced. */
	for(rp=lemp->rule; rp; rp=rp->next) rp->canReduce = B_FALSE;
	for(i=0; i<lemp->nstate; i++){
		struct action *ap;
		for(ap=lemp->sorted[i]->ap; ap; ap=ap->next){
			if (ap->type==REDUCE)  ap->x.rp->canReduce = B_TRUE;
		}
	}
	for(rp=lemp->rule; rp; rp=rp->next){
		if (rp->canReduce)  continue;
		ErrorMsg(lemp->filename,rp->ruleline,"This rule can not be reduced.\n");
		lemp->errorcnt++;
	}
}

/* Resolve a conflict between the two given actions.  If the
** conflict can't be resolve, return non-zero.
**
** NO LONGER TRUE:
**   To resolve a conflict, first look to see if either action
**   is on an error rule.  In that case, take the action which
**   is not associated with the error rule.  If neither or both
**   actions are associated with an error rule, then try to
**   use precedence to resolve the conflict.
**
** If either action is a SHIFT, then it must be apx.  This
** function won't work if apx->type==REDUCE and apy->type==SHIFT.
*/
static int resolve_conflict(
	struct action *apx,
	struct action *apy,
	struct symbol *errsym)   /* The error symbol (if defined.  NULL otherwise) */
{
	struct symbol *spx, *spy;
	int errcnt = 0;
	assert (apx->sp==apy->sp) ;  /* Otherwise there would be no conflict */
	if (apx->type==SHIFT && apy->type==REDUCE) {
		spx = apx->sp;
		spy = apy->x.rp->precsym;
		if (spy==0 || spx->prec<0 || spy->prec<0) {
			/* Not enough precedence information. */
			apy->type = CONFLICT;
			errcnt++;
		} else if (spx->prec>spy->prec) {    /* Lower precedence wins */
			apy->type = RD_RESOLVED;
		} else if (spx->prec<spy->prec) {
			apx->type = SH_RESOLVED;
		} else if (spx->prec==spy->prec && spx->assoc==RIGHT) { /* Use operator */
			apy->type = RD_RESOLVED;                             /* associativity */
		} else if (spx->prec==spy->prec && spx->assoc==LEFT) {  /* to break tie */
			apx->type = SH_RESOLVED;
		} else {
			assert (spx->prec==spy->prec && spx->assoc==NONE) ;
			apy->type = CONFLICT;
			errcnt++;
		}
	} else if (apx->type==REDUCE && apy->type==REDUCE) {
		spx = apx->x.rp->precsym;
		spy = apy->x.rp->precsym;
		if (spx==0 || spy==0 || spx->prec<0 ||
		spy->prec<0 || spx->prec==spy->prec) {
			apy->type = CONFLICT;
			errcnt++;
		} else if (spx->prec>spy->prec) {
			apy->type = RD_RESOLVED;
		} else if (spx->prec<spy->prec) {
			apx->type = RD_RESOLVED;
		}
	} else {
		assert (
			apx->type==SH_RESOLVED ||
			apx->type==RD_RESOLVED ||
			apx->type==CONFLICT ||
			apy->type==SH_RESOLVED ||
			apy->type==RD_RESOLVED ||
			apy->type==CONFLICT
	 ) ;
		/* The REDUCE/SHIFT case cannot happen because SHIFTs come before
		** REDUCEs on the list.  If we reach this point it must be because
		** the parser conflict had already been resolved. */
	}
	return errcnt;
}

/********************* From the file "configlist.c" *************************/
/*
** Routines to processing a configuration list and building a state
** in the LEMON parser generator.
*/

static struct config *freelist = 0;      /* List of free configurations */
static struct config *current = 0;       /* Top of list of configurations */
static struct config **currentend = 0;   /* Last on list of configs */
static struct config *basis = 0;         /* Top of list of basis configs */
static struct config **basisend = 0;     /* End of list of basis configs */

/* Return a pointer to a new configuration */
static struct config *newconfig() {
	struct config *new;
	if (freelist==0) {
		int i;
		int amt = 3;
		freelist = (struct config *)malloc (sizeof(struct config)*amt) ;
		if (freelist==0) {
			fprintf(stderr,"Unable to allocate memory for a new configuration.");
			exit(1);
		}
		for(i=0; i<amt-1; i++) freelist[i].next = &freelist[i+1];
		freelist[amt-1].next = 0;
	}
	new = freelist;
	freelist = freelist->next;
	return new;
}

/* The configuration "old" is no longer used */
static void deleteconfig(struct config *old)
{
	old->next = freelist;
	freelist = old;
}

/* Initialized the configuration list builder */
void Configlist_init(){
	current = 0;
	currentend = &current;
	basis = 0;
	basisend = &basis;
	Configtable_init();
	return;
}

/* Initialized the configuration list builder */
void Configlist_reset(){
	current = 0;
	currentend = &current;
	basis = 0;
	basisend = &basis;
	Configtable_clear(0);
	return;
}

/* Add another configuration to the configuration list */
struct config *Configlist_add(
	struct rule *rp,    /* The rule */
	int dot)            /* Index into the RHS of the rule where the dot goes */
{
	struct config *cfp, model;

	assert (currentend!=0) ;
	model.rp = rp;
	model.dot = dot;
	cfp = Configtable_find(&model);
	if (cfp==0) {
		cfp = newconfig();
		cfp->rp = rp;
		cfp->dot = dot;
		cfp->fws = SetNew();
		cfp->stp = 0;
		cfp->fplp = cfp->bplp = 0;
		cfp->next = 0;
		cfp->bp = 0;
		*currentend = cfp;
		currentend = &cfp->next;
		Configtable_insert(cfp);
	}
	return cfp;
}

/* Add a basis configuration to the configuration list */
struct config *Configlist_addbasis(struct rule *rp, int dot)
{
	struct config *cfp, model;

	assert (basisend!=0);
	assert (currentend!=0);
	model.rp = rp;
	model.dot = dot;
	cfp = Configtable_find(&model);
	if (cfp == 0) {
		cfp = newconfig();
		cfp->rp = rp;
		cfp->dot = dot;
		cfp->fws = SetNew();
		cfp->stp = 0;
		cfp->fplp = cfp->bplp = 0;
		cfp->next = 0;
		cfp->bp = 0;
		*currentend = cfp;
		currentend = &cfp->next;
		*basisend = cfp;
		basisend = &cfp->bp;
		Configtable_insert(cfp);
	}
	return cfp;
}

/* Compute the closure of the configuration list */
void Configlist_closure(struct lemon *lemp)
{
	struct config *cfp, *newcfp;
	struct rule *rp, *newrp;
	struct symbol *sp, *xsp;
	int i, dot;

	assert (currentend!=0) ;
	for(cfp=current; cfp; cfp=cfp->next){
		rp = cfp->rp;
		dot = cfp->dot;
		if (dot>=rp->nrhs)  continue;
		sp = rp->rhs[dot];
		if (sp->type==NONTERMINAL) {
			if (sp->rule==0 && sp!=lemp->errsym) {
				ErrorMsg(lemp->filename,rp->line,"Nonterminal \"%s\" has no rules.",
					sp->name);
				lemp->errorcnt++;
			}
			for(newrp=sp->rule; newrp; newrp=newrp->nextlhs){
				newcfp = Configlist_add(newrp,0);
				for (i=dot+1; i<rp->nrhs; i++) {
					xsp = rp->rhs[i];
					if (xsp->type==TERMINAL) {
						SetAdd(newcfp->fws,xsp->index);
						break;
					} else {
						SetUnion(newcfp->fws,xsp->firstset);
						if (xsp->lambda==B_FALSE)  break;
					}
				}
				if (i==rp->nrhs)  Plink_add(&cfp->fplp,newcfp);
			}
		}
	}
	return;
}

/* Sort the configuration list */
void Configlist_sort() {
	current = (struct config *)msort((char *)current,(char **)&(current->next),Configcmp);
	currentend = 0;
	return;
}

/* Sort the basis configuration list */
void Configlist_sortbasis() {
	basis = (struct config *)msort((char *)current,(char **)&(current->bp),Configcmp);
	basisend = 0;
	return;
}

/* Return a pointer to the head of the configuration list and
** reset the list */
struct config *Configlist_return() {
	struct config *old;
	old = current;
	current = 0;
	currentend = 0;
	return old;
}

/* Return a pointer to the head of the configuration list and
** reset the list */
struct config *Configlist_basis() {
	struct config *old;
	old = basis;
	basis = 0;
	basisend = 0;
	return old;
}

/* Free all elements of the given configuration list */
void Configlist_eat(struct config *cfp)
{
	struct config *nextcfp;
	for(; cfp; cfp=nextcfp){
		nextcfp = cfp->next;
		assert (cfp->fplp==0) ;
		assert (cfp->bplp==0) ;
		if (cfp->fws)  SetFree(cfp->fws);
		deleteconfig(cfp);
	}
	return;
}

/**************** From the file "main.c" ************************************/
/*
** Main program file for the LEMON parser generator.
*/

static int nDefine = 0;      /* Number of -D options on the command line */
static char **azDefine = 0;  /* Name of the -D macros */

/* This routine is called with the argument to each -D command-line option.
** Add the macro defined to the azDefine array.
*/
static void handle_D_option(char *z) {
	char **paz;
	nDefine++;
	azDefine = realloc(azDefine, sizeof(azDefine[0])*nDefine);
	if (azDefine==0) {
		fprintf(stderr,"out of memory\n");
		exit(1);
	}
	paz = &azDefine[nDefine-1];
	*paz = malloc (strlen(z)+1) ;
	if (*paz==0) {
		fprintf(stderr,"out of memory\n");
		exit(1);
	}
	strcpy(*paz, z);
	for(z=*paz; *z && *z!='='; z++){}
	*z = 0;
}


/* The main program.  Parse the command line and do it... */
int main(int argc, char **argv) {
	static int version    = 0;
	static int rpflag     = 0;
	static int basisflag  = 0;
	static int compress   = 0;
	static int quiet      = 0;
	static int statistics = 0;
	static int mhflag     = 0;

	static struct s_options options[] = {
		{OPT_FLAG, "b", (char*)&basisflag,      "Print only the basis in report."},
		{OPT_FLAG, "c", (char*)&compress,       "Don't compress the action table."},
		{OPT_FSTR, "D", (char*)handle_D_option, "Define an %ifdef macro."},
		{OPT_FLAG, "g", (char*)&rpflag,         "Print grammar without actions."},
		{OPT_FLAG, "m", (char*)&mhflag,         "Output a makeheaders compatible file"},
		{OPT_FLAG, "q", (char*)&quiet,          "(Quiet) Don't print the report file."},
		{OPT_FLAG, "s", (char*)&statistics,     "Print parser stats to standard output."},
		{OPT_FLAG, "x", (char*)&version,        "Print the version number."},
		{OPT_FLAG,0,0,0}
	};
	int i;
	struct lemon lem;

	OptInit(argv,options,stderr);
	if (version) {
		 printf("Lemon version 1.0\n");
		 exit(0);
	}
	if (OptNArgs()!=1) {
		fprintf(stderr,"Exactly one filename argument is required.\n");
		exit(1);
	}
	lem.errorcnt = 0;

	/* Initialize the machine */
	Strsafe_init();
	Symbol_init();
	State_init();
	lem.argv0 = argv[0];
	lem.filename = OptArg(0);
	lem.basisflag = basisflag;
	lem.has_fallback = 0;
	lem.nconflict = 0;
	lem.name = lem.include = lem.arg = lem.tokentype = lem.start = 0;
	lem.vartype = 0;
	lem.stacksize = 0;
	lem.error = lem.overflow = lem.failure = lem.accept = lem.tokendest =
		 lem.tokenprefix = lem.outname = lem.extracode = 0;
	lem.vardest = 0;
	lem.tablesize = 0;
	Symbol_new("$");
	lem.errsym = Symbol_new("error");

	/* Parse the input file */
	Parse(&lem, nDefine, azDefine);
	if (lem.errorcnt)  exit(lem.errorcnt);
	if (lem.rule==0) {
		fprintf(stderr,"Empty grammar.\n");
		exit(1);
	}

	/* Count and index the symbols of the grammar */
	lem.nsymbol = Symbol_count();
	Symbol_new("{default}");
	lem.symbols = Symbol_arrayof();
	for(i=0; i<=lem.nsymbol; i++) lem.symbols[i]->index = i;
	qsort(lem.symbols,lem.nsymbol+1,sizeof(struct symbol*),
				(int(*)())Symbolcmpp);
	for(i=0; i<=lem.nsymbol; i++) lem.symbols[i]->index = i;
	for(i=1; isupper(lem.symbols[i]->name[0]); i++);
	lem.nterminal = i;

	/* Generate a reprint of the grammar, if requested on the command line */
	if (rpflag) {
		Reprint(&lem);
	} else {
		/* Initialize the size for all follow and first sets */
		SetSize(lem.nterminal);

		/* Find the precedence for every production rule (that has one) */
		FindRulePrecedences(&lem);

		/* Compute the lambda-nonterminals and the first-sets for every
		** nonterminal */
		FindFirstSets(&lem);

		/* Compute all LR(0) states.  Also record follow-set propagation
		** links so that the follow-set can be computed later */
		lem.nstate = 0;
		FindStates(&lem);
		lem.sorted = State_arrayof();

		/* Tie up loose ends on the propagation links */
		FindLinks(&lem);

		/* Compute the follow set of every reducible configuration */
		FindFollowSets(&lem);

		/* Compute the action tables */
		FindActions(&lem);

		/* Compress the action tables */
		if (compress==0)  CompressTables(&lem);

		/* Generate a report of the parser generated.  (the "y.output" file) */
		if (!quiet)  ReportOutput(&lem);

		/* Generate the source code for the parser */
		ReportTable(&lem, mhflag);

		/* Produce a header file for use by the scanner.  (This step is
		** omitted if the "-m" option is used because makeheaders will
		** generate the file for us.) */
		if (!mhflag)  ReportHeader(&lem);
	}
	if (statistics) {
		printf("Parser statistics: %d terminals, %d nonterminals, %d rules\n",
			lem.nterminal, lem.nsymbol - lem.nterminal, lem.nrule);
		printf("                   %d states, %d parser table entries, %d conflicts\n",
			lem.nstate, lem.tablesize, lem.nconflict);
	}
	if (lem.nconflict) {
		fprintf(stderr,"%d resolvable parsing conflicts.\n",lem.nconflict);
	}
	//exit(lem.errorcnt + lem.nconflict);
	// JRK 2016-05-23
	// According to the manual, these conflicts are resolved by taking the first match.
	// This is OK for my purposes.
	exit(lem.errorcnt);
	return (lem.errorcnt + lem.nconflict);
}

/******************** From the file "msort.c" *******************************/
/*
** A generic merge-sort program.
**
** USAGE:
** Let "ptr" be a pointer to some structure which is at the head of
** a null-terminated list.  Then to sort the list call:
**
**     ptr = msort(ptr,&(ptr->next),cmpfnc);
**
** In the above, "cmpfnc" is a pointer to a function which compares
** two instances of the structure and returns an integer, as in
** strcmp.  The second argument is a pointer to the pointer to the
** second element of the linked list.  This address is used to compute
** the offset to the "next" field within the structure.  The offset to
** the "next" field must be constant for all structures in the list.
**
** The function returns a new pointer which is the head of the list
** after sorting.
**
** ALGORITHM:
** Merge-sort.
*/

/*
** Return a pointer to the next structure in the linked list.
*/
#define NEXT(A) (*(char**)(((unsigned long)A)+offset))

/*
** Inputs:
**   a:       A sorted, null-terminated linked list.  (May be null).
**   b:       A sorted, null-terminated linked list.  (May be null).
**   cmp:     A pointer to the comparison function.
**   offset:  Offset in the structure to the "next" field.
**
** Return Value:
**   A pointer to the head of a sorted list containing the elements
**   of both a and b.
**
** Side effects:
**   The "next" pointers for elements in the lists a and b are
**   changed.
*/
static char *merge(
	char *a,
	char *b,
	int (*cmp)(),
	int offset)
{
	char *ptr, *head;

	if (a==0) {
		head = b;
	} else if (b==0) {
		head = a;
	} else {
		if ((*cmp)(a,b)<0) {
			ptr = a;
			a = NEXT(a);
		} else {
			ptr = b;
			b = NEXT(b);
		}
		head = ptr;
		while (a && b) {
			if ((*cmp)(a,b)<0) {
				NEXT(ptr) = a;
				ptr = a;
				a = NEXT(a);
			} else {
				NEXT(ptr) = b;
				ptr = b;
				b = NEXT(b);
			}
		}
		if (a)  NEXT(ptr) = a;
		else    NEXT(ptr) = b;
	}
	return head;
}

/*
** Inputs:
**   list:      Pointer to a singly-linked list of structures.
**   next:      Pointer to pointer to the second element of the list.
**   cmp:       A comparison function.
**
** Return Value:
**   A pointer to the head of a sorted list containing the elements
**   orginally in list.
**
** Side effects:
**   The "next" pointers for elements in list are changed.
*/
#define LISTSIZE 30
char *msort(char *list, char **next, int (*cmp)())
{
	unsigned long offset;
	char *ep;
	char *set[LISTSIZE];
	int i;
	offset = (unsigned long)next - (unsigned long)list;
	for(i=0; i<LISTSIZE; i++) set[i] = 0;
	while (list) {
		ep = list;
		list = NEXT(list);
		NEXT(ep) = 0;
		for(i=0; i<LISTSIZE-1 && set[i]!=0; i++){
			ep = merge(ep,set[i],cmp,offset);
			set[i] = 0;
		}
		set[i] = ep;
	}
	ep = 0;
	for(i=0; i<LISTSIZE; i++) if (set[i])  ep = merge(ep,set[i],cmp,offset);
	return ep;
}

/***************** From the file "set.c" ************************************/

/*
** Set manipulation routines for the LEMON parser generator.
*/

static int size = 0;

/* Set the set size */
void SetSize(n)
int n;
{
	size = n+1;
}

/* Allocate a new set */
char *SetNew(){
	char *s;
	int i;
	s = (char*)malloc (size) ;
	if (s==0) {
		extern void memory_error();
		memory_error();
	}
	for(i=0; i<size; i++) s[i] = 0;
	return s;
}

/* Deallocate a set */
void SetFree(s)
char *s;
{
	free(s);
}

/* Add a new element to the set.  Return TRUE if the element was added
** and FALSE if it was already there. */
int SetAdd(char *s, int e)
{
	int rv;
	rv = s[e];
	s[e] = 1;
	return !rv;
}

/* Add every element of s2 to s1.  Return TRUE if s1 changes. */
int SetUnion(char *s1, char *s2)
{
	int i, progress;
	progress = 0;
	for(i=0; i<size; i++){
		if (s2[i]==0)  continue;
		if (s1[i]==0) {
			progress = 1;
			s1[i] = 1;
		}
	}
	return progress;
}

/********************** From the file "table.c" ****************************/
/*
** All code in this file has been automatically generated
** from a specification in the file
**              "table.q"
** by the associative array code building program "aagen".
** Do not edit this file!  Instead, edit the specification
** file, then rerun aagen.
*/
/*
** Code for processing tables in the LEMON parser generator.
*/

/* Compare two configurations */
int Configcmp(a,b)
struct config *a;
struct config *b;
{
	int x;
	x = a->rp->index - b->rp->index;
	if (x==0)  x = a->dot - b->dot;
	return x;
}

/* Compare two states */
static int statecmp(a,b)
struct config *a;
struct config *b;
{
	int rc;
	for(rc=0; rc==0 && a && b;  a=a->bp, b=b->bp){
		rc = a->rp->index - b->rp->index;
		if (rc==0)  rc = a->dot - b->dot;
	}
	if (rc==0) {
		if (a)  rc = 1;
		if (b)  rc = -1;
	}
	return rc;
}

/* Hash a state */
static int statehash(a)
struct config *a;
{
	int h=0;
	while (a) {
		h = h*571 + a->rp->index*37 + a->dot;
		a = a->bp;
	}
	return h;
}

/* Allocate a new state structure */
struct state *State_new()
{
	struct state *new;
	new = (struct state *)malloc (sizeof(struct state)) ;
	MemoryCheck(new);
	return new;
}

/* There is one instance of the following structure for each
** associative array of type "x3".
*/
struct s_x3 {
	int size;               /* The number of available slots. Must be a power of 2, >= 1. */
	int count;              /* Number of currently slots filled */
	struct s_x3node *tbl;  /* The data stored here */
	struct s_x3node **ht;  /* Hash table for lookups */
};

/* There is one instance of this structure for every data element
** in an associative array of type "x3".
*/
typedef struct s_x3node {
	struct state *data;                  /* The data */
	struct config *key;                   /* The key */
	struct s_x3node *next;   /* Next entry with the same hash */
	struct s_x3node **from;  /* Previous link */
} x3node;

/* There is only one instance of the array, which is the following */
static struct s_x3 *x3a;

/* Allocate a new associative array */
void State_init(){
	if (x3a)  return;
	x3a = (struct s_x3*)malloc (sizeof(struct s_x3)) ;
	if (x3a) {
		x3a->size = 128;
		x3a->count = 0;
		x3a->tbl = (x3node*)malloc (
			(sizeof(x3node) + sizeof(x3node*))*128) ;
		if (x3a->tbl==0) {
			free(x3a);
			x3a = 0;
		} else {
			int i;
			x3a->ht = (x3node**)&(x3a->tbl[128]);
			for(i=0; i<128; i++) x3a->ht[i] = 0;
		}
	}
}

/* Insert a new record into the array.  Return TRUE if successful.
** Prior data with the same key is NOT overwritten */
int State_insert(struct state *data, struct config *key)
{
	x3node *np;
	int h;
	int ph;

	if (x3a==0)  return 0;
	ph = statehash(key);
	h = ph & (x3a->size-1);
	np = x3a->ht[h];
	while (np) {
		if (statecmp(np->key,key)==0) {
			/* An existing entry with the same key is found. */
			/* Fail because overwrite is not allows. */
			return 0;
		}
		np = np->next;
	}
	if (x3a->count>=x3a->size) {
		/* Need to make the hash table bigger */
		int i,size;
		struct s_x3 array;
		array.size = size = x3a->size*2;
		array.count = x3a->count;
		array.tbl = (x3node*)malloc(
			(sizeof(x3node) + sizeof(x3node*))*size) ;
		if (array.tbl==0)  return 0;  /* Fail due to malloc failure */
		array.ht = (x3node**)&(array.tbl[size]);
		for(i=0; i<size; i++) array.ht[i] = 0;
		for(i=0; i<x3a->count; i++){
			x3node *oldnp, *newnp;
			oldnp = &(x3a->tbl[i]);
			h = statehash(oldnp->key) & (size-1);
			newnp = &(array.tbl[i]);
			if (array.ht[h])  array.ht[h]->from = &(newnp->next);
			newnp->next = array.ht[h];
			newnp->key = oldnp->key;
			newnp->data = oldnp->data;
			newnp->from = &(array.ht[h]);
			array.ht[h] = newnp;
		}
		free(x3a->tbl);
		*x3a = array;
	}
	/* Insert the new data */
	h = ph & (x3a->size-1);
	np = &(x3a->tbl[x3a->count++]);
	np->key = key;
	np->data = data;
	if (x3a->ht[h])  x3a->ht[h]->from = &(np->next);
	np->next = x3a->ht[h];
	x3a->ht[h] = np;
	np->from = &(x3a->ht[h]);
	return 1;
}

/* Return a pointer to data assigned to the given key.  Return NULL
** if no such key. */
struct state *State_find(struct config *key)
{
	int h;
	x3node *np;

	if (x3a==0)  return 0;
	h = statehash(key) & (x3a->size-1);
	np = x3a->ht[h];
	while (np) {
		if (statecmp(np->key,key)==0)  break;
		np = np->next;
	}
	return np ? np->data : 0;
}

/* Return an array of pointers to all data in the table.
** The array is obtained from malloc.  Return NULL if memory allocation
** problems, or if the array is empty. */
struct state **State_arrayof()
{
	struct state **array;
	int i,size;
	if (x3a==0)  return 0;
	size = x3a->count;
	array = (struct state **)malloc (sizeof(struct state *)*size) ;
	if (array) {
		for(i=0; i<size; i++) array[i] = x3a->tbl[i].data;
	}
	return array;
}

/* Hash a configuration */
static int confighash(a)
struct config *a;
{
	int h=0;
	h = h*571 + a->rp->index*37 + a->dot;
	return h;
}

/* There is one instance of the following structure for each
** associative array of type "x4".
*/
struct s_x4 {
	int size;               /* The number of available slots.  Must be a power of 2, >= 1. */
	int count;              /* Number of currently slots filled */
	struct s_x4node *tbl;  /* The data stored here */
	struct s_x4node **ht;  /* Hash table for lookups */
};

/* There is one instance of this structure for every data element
** in an associative array of type "x4".
*/
typedef struct s_x4node {
	struct config *data;     /* The data */
	struct s_x4node *next;   /* Next entry with the same hash */
	struct s_x4node **from;  /* Previous link */
} x4node;

/* There is only one instance of the array, which is the following */
static struct s_x4 *x4a;

/* Allocate a new associative array */
void Configtable_init() {
	if (x4a)  return;
	x4a = (struct s_x4*)malloc (sizeof(struct s_x4)) ;
	if (x4a) {
		x4a->size = 64;
		x4a->count = 0;
		x4a->tbl = (x4node*)malloc (
			(sizeof(x4node) + sizeof(x4node*))*64) ;
		if (x4a->tbl==0) {
			free(x4a);
			x4a = 0;
		} else {
			int i;
			x4a->ht = (x4node**)&(x4a->tbl[64]);
			for(i=0; i<64; i++) x4a->ht[i] = 0;
		}
	}
}

/* Insert a new record into the array.  Return TRUE if successful.
** Prior data with the same key is NOT overwritten */
int Configtable_insert(struct config *data) {
	x4node *np;
	int h;
	int ph;

	if (x4a==0)  return 0;
	ph = confighash(data);
	h = ph & (x4a->size-1);
	np = x4a->ht[h];
	while (np) {
		if (Configcmp(np->data,data)==0) {
			/* An existing entry with the same key is found. */
			/* Fail because overwrite is not allows. */
			return 0;
		}
		np = np->next;
	}
	if (x4a->count>=x4a->size) {
		/* Need to make the hash table bigger */
		int i,size;
		struct s_x4 array;
		array.size = size = x4a->size*2;
		array.count = x4a->count;
		array.tbl = (x4node*)malloc(
			(sizeof(x4node) + sizeof(x4node*))*size) ;
		if (array.tbl==0)  return 0;  /* Fail due to malloc failure */
		array.ht = (x4node**)&(array.tbl[size]);
		for(i=0; i<size; i++) array.ht[i] = 0;
		for(i=0; i<x4a->count; i++){
			x4node *oldnp, *newnp;
			oldnp = &(x4a->tbl[i]);
			h = confighash(oldnp->data) & (size-1);
			newnp = &(array.tbl[i]);
			if (array.ht[h])  array.ht[h]->from = &(newnp->next);
			newnp->next = array.ht[h];
			newnp->data = oldnp->data;
			newnp->from = &(array.ht[h]);
			array.ht[h] = newnp;
		}
		free(x4a->tbl);
		*x4a = array;
	}
	/* Insert the new data */
	h = ph & (x4a->size-1);
	np = &(x4a->tbl[x4a->count++]);
	np->data = data;
	if (x4a->ht[h])  x4a->ht[h]->from = &(np->next);
	np->next = x4a->ht[h];
	x4a->ht[h] = np;
	np->from = &(x4a->ht[h]);
	return 1;
}

/* Return a pointer to data assigned to the given key.  Return NULL
** if no such key. */
struct config *Configtable_find(struct config *key)
{
	int h;
	x4node *np;

	if (x4a==0)  return 0;
	h = confighash(key) & (x4a->size-1);
	np = x4a->ht[h];
	while (np) {
		if (Configcmp(np->data,key)==0)  break;
		np = np->next;
	}
	return np ? np->data : 0;
}

/* Remove all data from the table.  Pass each data to the function "f"
** as it is removed.  ("f" may be null to avoid this step.) */
void Configtable_clear(int(*f)(/* struct config * */))
{
	int i;
	if (x4a==0 || x4a->count==0)  return;
	if (f)  for(i=0; i<x4a->count; i++) (*f)(x4a->tbl[i].data);
	for(i=0; i<x4a->size; i++) x4a->ht[i] = 0;
	x4a->count = 0;
	return;
}
