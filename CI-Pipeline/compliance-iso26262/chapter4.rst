Software Verification Report
============================

Clauses Addressed
-----------------

ISO 26262:6:2018, Clause 9, Table 7 (*Methods for software unit verification*)

	| 1f Control flow analysis
	| 1g Data flow analysis
	| 1h Static code analysis
	| 1j Requirements based test
	| 1k Interface test
	| 1l Fault injection test

ISO 26262:6:2018, Clause 9, Table 9 (*Structural coverage metrics at the software unit level*)

	| 1a Statement coverage
	| 1b Branch coverage
	| 1c MC/DC coverage (Modified Condition/Decision Coverage)

Clauses Not Addressed
---------------------

ISO 26262:6:2018, Clause 9, Table 7 (*Methods for software unit verification*)

	| 1a Walk-through
	| 1b Pair-programming
	| 1c Inspection
	| 1e Formal verification
	| 1i Static analyses based on abstract interpretation
	| 1m Resource usage evaluation
	| 1n Back-to-back comparison test between model and code

These clauses are not addressed because they are not applicable in the context of Apex.OS software.

Control Flow Analysis
---------------------

Data Flow Analysis
------------------

Here is the DSM for the package itself:

.. image:: _assets/lattix/dsm.png

Static Code Analysis Report
---------------------------

Interface and Fault Injection Test Report
-----------------------------------------

Statement, Decision, MC/DC Coverage Report
------------------------------------------

Unit Test Report
----------------
