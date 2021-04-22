#!/usr/bin/env rfgscript

#  Axivion Bauhaus Suite
#  http://www.axivion.com/
#  Copyright (C) Axivion GmbH, 2019

'''
A simple RFG to LDI conversion tool.
'''

try:
    import bauhaus

    del bauhaus
except ImportError:
    import sys, subprocess

    sys.exit(subprocess.call(['rfgscript'] + sys.argv))

from bauhaus import rfg
from bauhaus import ir
import sys
import re
import os

def get_source_name(node):
    if 'Source.Name' in node:
        return node['Source.Name']
    else:
        return '?'

DELIM = '@'

# &amp must be first so that it does not replace the & in the previous replacements
XMLIFY = [('&', '&amp;'), ('<', '&lt;'), ('>', '&gt;'), ('"', '&quot;')]

TYPE_REF_SET = {"Local_Var_Of_Type", "Alignof_Of_Type", "Cast_Of_Type", "Of_Type", "Parameter_Of_Type", "Return_Type", "Sizeof_Of_Type", "Type_Synonym_To", "Typeid_Of_Type" }
SKIP_TYPE_SET = {"Declare", "Declared_In", "Method_Address", "Routine_Address" }

DEP_TYPE_MAP = {
	'Dispatching_Call': 'Invokes.Virtual',
	'Implicit_Static_Call': 'Invokes.Static',
	'Static_Call': 'Invokes.Static',
	'Implicit_Dispatching_Call': 'Invokes.Dispatch_Call',
	'Member_Address': 'Member.Address',
	'Member_Set': 'Member.Set',
	'Member_Use': 'Member.Use',
	'Variable_Address': 'Variable.Address',
   'Variable_Deref': 'Variable.Deref',
   'Variable_Set': 'Variable.Set',
   'Variable_Use': 'Variable.Use',	
   'Inherit': 'Type.Inherit',	
   'Implementation_Of': 'Type.Implementation',	
   'Instantiate': 'Type.Instantiate',
   'Template_Argument': 'Type.Reference',	
   'Throw': 'Type.Reference',	
   'Specialization_Of': 'Type.Reference',	
   'New_Of_Type': 'Type.Constructs',	
   'Extend': 'Type.Extends',	
   'Catch': 'Type.Reference',	
   'Enumerator_Use': 'Type.Reference',
   'Friend_Declare': 'Type.Reference',
   'Grant_Friendship_To': 'Type.Reference',
   'Override': 'Type.Override',
   'New_Of_Type': 'Type.Instantiate',
	}


def xmlify_string (str):
    for pair in XMLIFY:
        str = str.replace(pair[0], pair[1])
    return str
   
NAMESPACED = dict()
   
def strip_atsigns(str):
   rep = "@"
   len2 = len(rep)
   index = str.find(rep)
   if (index != -1):
      start = str[0:index];
      end = str[index+len2:]
      index2 = end.find("dll.ir");
      if (index2 != -1):
         end = end[index2+6:]
         str2 = start + strip_atsigns(end)
#         print str2
         return str2
         
   return str;

def namespaced_name(node, ir_graph, hierarchy):

    if node in NAMESPACED:
        return NAMESPACED[node]
        
    if (node.is_of_subtype("Routine") or node.is_of_subtype("Method")) and ir_graph:
       
        pir_node = ir_graph.node(node)
        
        result = None
        name = None
        
        if 'Logical' in pir_node.fields() and pir_node.Logical:
           result = pir_node.Logical.Full_Name
           name = pir_node.Logical.Name
        elif 'Physical' in pir_node.fields() and pir_node.Physical:
           p = pir_node.Physical
           if 'Logical' in p.fields() and p.Logical:
              result = p.Logical.Full_Name
              name = p.Logical.Name
        
        if not result:   
           if 'Full_Name' in pir_node.fields() and pir_node.Full_Name:
              result = pir_node.Full_Name
              name = get_source_name(node)
            
        if result:
#            print("===================")
#            print("result  %s" % result)
#            print("result0 %s\n" % name)
            
            if re.match('^routine ', result):
               result = result[8:]
            elif re.match('^inline ', result):
               result = result[7:]
            
            result = result.strip();
            index = result.find(name + '(')
            
            index = result.find(')')
            returnval = ''
            if index != -1:
               rest = result[index+1:]
               result = result[0:index+1]
               
               index2 = rest.find('->')
               
               if index2 != -1:
                  returnval = rest[index2+2:];
                  index3 = returnval.find('-throw(')
                  if index3 != -1:
                     returnval = returnval[0:index3]
                  returnval += ' '
#                  print('returnval %s' %returnval)   
#               print('rest %s' %rest)
            
                        
            result = returnval + result   
            
            result = strip_atsigns(result);
            
#            if (result.find(".dll.ir") != -1):
#               print result
               
#            print("result2 %s\n" % result)

            #ir.unparse_lir(pir_node.Logical)
        else:
            result = get_source_name(node)
    else:
        result = get_source_name(node)
    
    n = node
    while rfg.hierarchies.has_parent(hierarchy, n):
        p = rfg.hierarchies.parent(hierarchy, n)
        result = '%s%s%s' % (get_source_name(p), DELIM, result)
        n = p
    result = xmlify_string(result)
    NAMESPACED[node] = result
    return result
    

def main(ldi, graph, ir_graph, view, hierarchy, ldi_filename, typeSet, skip):
    
#    for nodetype in ['Member', 'Method']:
#        ldi.write('    <elementtype type="%s" member="true" />' % nodetype)
        
    for node in view.xnodes():
        name = namespaced_name(node, ir_graph, hierarchy)
        type = node.node_type().name()
        
        if typeSet:
          if (skip):
             if (not type in typeSet):
           	  	 continue
          else:
             if (type in typeSet):
           	  	 continue
        
        if type == 'File':
           if re.match('^.*\.(h|hpp|hxx|hh|inc|inl)$', name.lower()):
        	     type = 'Header_File'
           else:
        	     type = 'Source_File'
        elif type == 'Directory':
           type = None
        elif type == 'Namespace':
           type = None
        elif type == 'System':
           type = None
                      
        if (type):
	        ldi.write('    <element \n')
	        ldi.write('        name="%s"\n' % name)
	        ldi.write('        type="%s"\n' % type)
	        ldi.write('    >\n')
	        #for property in ['Source.File', 'Source.Path', 'Source.Line', 'Source.Column']:
	        #    if property in node:
	        #        ldi.write('        <property name="%s">%s</property>\n' % (property, node[property]))
	        atomsource = None
	        atompath = None
	        atomlinenumber = None
	        atomsourcefile = None
	        if 'Source.File' in node:
	           atomsource = node['Source.File']
	        if 'Source.Path' in node:
	           atompath = node['Source.Path']
	        #if 'Source.Line' in node:
	        #   atomlinenumber = node['Source.Line']
	        if atomsource and atompath:
	           atomsourcefile = os.path.join(atompath, atomsource)
	        elif atomsource:
	           atomsourcefile = atomsource
	        
	        if atomlinenumber:
	           ldi.write('            <property name="linenumber">%s</property>\n' % atomlinenumber)
	        if atomsourcefile:
	           ldi.write('            <property name="sourcefile">%s</property>\n' % atomsourcefile)
		              	                
	        for edge in node.outgoings(view):
	            orig_edge_type = edge.edge_type().name();
	            edge_type = orig_edge_type
	            
	            try:
	               edge_type2 = DEP_TYPE_MAP[edge_type]
	               if edge_type2:
	                  edge_type = edge_type2
	            except:
	               pass
	            
	            if (edge_type in TYPE_REF_SET):
	               edge_type = 'Type.Reference'
	            if (edge_type in SKIP_TYPE_SET):
	               edge_type = None
	            
	                               
	            if edge_type and not edge.is_of_subtype('Belongs_To'):
	                edge_name = namespaced_name(edge.target(), ir_graph, hierarchy)
	                
	                if re.match('^.*\.(h|hpp|hxx|hh|inc|inl).*', name.lower()):
	                   if re.match('^.*\.(c|cpp).*', edge_name.lower()):
	                      if (orig_edge_type == 'Override' or orig_edge_type == 'Inherit'):
	                         edge_name = None
	                         
	                if edge_name:
		                ldi.write('        <uses \n')
		                ldi.write('            provider="%s"\n' % edge_name)
		                ldi.write('            kind="%s"\n' % edge_type)
		                ldi.write('        >\n')
		                # properties
		                
		                source = None
		                path = None
		                linenumber = None
		                sourcefile = None
		                
		                if 'Source.File' in edge:
		                    source = edge['Source.File']
		                if 'Source.Path' in edge:
		                    path = edge['Source.Path']
		                if 'Source.Line' in edge:
		                    linenumber = edge['Source.Line']
		                
		                if source and path:
		                    sourcefile = os.path.join(path, source)
		                elif source:
		                    sourcefile = source
		                    
		                if linenumber:
		                    ldi.write('            <property name="linenumber">%s</property>\n' % linenumber)
		                if sourcefile:
		                    ldi.write('            <property name="sourcefile">%s</property>\n' % sourcefile)
		              
		                                
		#                for property in ['Source.File', 'Source.Path', 'Source.Line', 'Source.Column']:
		#                    if property in edge:
		#                        ldi.write('            <property name="%s">%s</property>\n' % (property, edge[property]))
		                ldi.write('        </uses>')
		                
	        ldi.write('    </element>\n')

if __name__ == '__main__':
    rfgfile = None
    irfile = None
    viewname = None
    hierarchy = None
    ldifile = None
    
    irfileindex = -1
    
    for i in range(1, len(sys.argv)):
        arg = sys.argv[i];
        
        if i == irfileindex:
          irfile = arg
          irfileindex = -1
        elif i == 1:
            rfgfile = arg
        elif (arg == '-i'):
            print("found -i")
            irfileindex = i+1;
        elif viewname is None:
           viewname = arg
        elif hierarchy is None:
           hierarchy = arg     
        elif ldifile is None:
           ldifile = arg           
        
    
    print ('rfgfile %s ' % rfgfile)
    print ('irfile %s ' % irfile)
    print('viewname %s' % viewname)
    print('hierarchy %s' % hierarchy)
    print('ldifile %s' % ldifile)
    
    if not rfgfile or not ldifile or not viewname or not hierarchy:
        print("usage: %s <rfgfile> <irfile> <viewname> <hierarchy> <ldifile>" % sys.argv[0])
        sys.exit(1)

    graph = rfg.Graph(rfgfile)
    ir_graph = None
    
     
    if irfile:
        ir_graph = ir.Graph(irfile)
        ir_graph.connect(graph)
#        print(ir_graph.Language)
        
    if not graph.is_view_name(viewname):
        print('error: view "%s" not found' % viewname)
        sys.exit(1)
    view = graph.view(viewname)
    if not graph.is_view_name(hierarchy):
        print('error: view "%s" not found' % hierarchy)
        sys.exit(1)
    hierarchy = graph.view(hierarchy)
    
    ldi = open(ldifile, "w")
    ldi.write('<?xml version="1.0" ?>\n')
    ldi.write('<ldi delimiter="%s">\n' % DELIM)
 
#    typeSet = {"Namespace", "Class", "File" }
#    main(ldi, graph, ir_graph, view, hierarchy, ldifile, typeSet, True)
#    main(ldi, graph, ir_graph, view, hierarchy, ldifile, typeSet, False)
    main(ldi, graph, ir_graph, view, hierarchy, ldifile, None, False)

    ldi.write('</ldi>\n')
    ldi.close()
    