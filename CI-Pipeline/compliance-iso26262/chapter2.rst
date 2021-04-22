Software Unit Design Specification
==================================

Clauses Addressed
-----------------

ISO 26262:6:2018, Clause 8:Table 5 (*Notations for software unit design*)

	| 1c Semi-formal notations (including UML diagrams)

ISO 26262:6:2018, Clause 8:Table 6 (*Design principles for software unit design and implementation*)

	| 1b No dynamic objects or variables, or else online test during their creation
	| 1c Initialization of variables
	| 1d No multiple use of variable names
	| 1e Avoid global variables, or justify their usage
	| 1g No implicit type conversions
	| 1i No unconditional jumps
	| 1j No recursions

Clauses Not Checked Using Tools
-------------------------------

ISO 26262:6:2018, Clause 8:Table 6 (*Design principles for software unit design and implementation*)

	| 1a One entry and one exit point in subprograms and functions - due to AUTOSAR [#]_ not enforcing it, and the use of C++ exceptions, which are technically exit points.
	| 1f Restricted use of pointers- we do use pointers, but only smart pointers.
	| 1h No hidden data flow or control flow - verified by examining the control flow diagrams

.. [#] AUTOSAR = Guidelines for the use of C++14 language in critical and safety-related systems, Document Identification number 839, Final Adaptive Platform, Standard Release 19-03.

Semi-formal notations (UML Diagrams)
------------------------------------

.. TODO

Design Principles
-----------------

| Note: |project_name| Suppression Report is:
| Manually checked for that either
| (a) the target rule is not suppressed, or
| (b) the target rule is suppressed but there exists a good justification for suppression.

Dynamic Memory Allocation
^^^^^^^^^^^^^^^^^^^^^^^^^

.. TODO

Initialization of Variables
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. TODO

Variable Names
^^^^^^^^^^^^^^

.. TODO

Global Variable Report
^^^^^^^^^^^^^^^^^^^^^^

.. TODO

Type Conversion Report
^^^^^^^^^^^^^^^^^^^^^^

.. TODO

Unconditional Jumps, Goto Analysis
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. TODO

Recursion Report
^^^^^^^^^^^^^^^^

.. TODO

