###################################################################
#                                                                 #
#   DYNNAMIC GRAPH CUTS - software for dynamically computing      #
#		mincut/maxflow in a graph   			  #
#                        Version 2	                          #
#  		(based on Ver 3.0 of MAXFLOW Code of              #
#                     						  #
#    http://www.cs.adastral.ucl.ac.uk/~vnk/software.html          #
#                                                                 #
#    Yuri Boykov (yuri@csd.uwo.ca)                                #
#    Vladimir Kolmogorov (vnk@adastral.ucl.ac.uk)                 #
#    2001-2006							  #
#                                                                 #
#                                                                 #
#         Pushmeet Kohli 		    Philip H.S. Torr  	  #
#  (pushmeet.kohli@brookes.ac.uk)   (philiptorr@brookes.ac.uk)    #
###################################################################

1. Introduction.

This software library implements the dynamic maxflow algorithm
described in

	Efficiently Solving Dynamic Markov Random Fields
	using Graph Cuts

	In the Tenth IEEE International Conference on Computer Vision
	(ICCV 2005)


The algorithm uses the maxflow algorithm code described in: 

	An Experimental Comparison of Min-Cut/Max-Flow Algorithms
	for Energy Minimization in Vision.
	Yuri Boykov and Vladimir Kolmogorov.
	In IEEE Transactions on Pattern Analysis and Machine Intelligence (PAMI), 
	September 2004

We thank Yuri Boykov and Vladimir Kolomogorov for providing the code.
If you use this software for research purposes, you should cite
the aforementioned papers in any resulting publication.


##################################################################

2. License.

    This source code also comes under the following license:

    Copyright 2001 Vladimir Kolmogorov (vnk@cs.cornell.edu), Yuri Boykov (yuri@csd.uwo.ca).


    Copyright 2005 Pushmeet Kohli (pushmeet.kohli@brookes.ac.uk),
		   Philip HS Torr (philiptorr@brookes.ac.uk).
        
    This software can be used for research purposes only.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
    A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
    OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
    LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
    DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
    THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

##################################################################

3. Graph representation.

The graph is represented as a adjacency list.

##################################################################

4. Example usage.

This section shows how to use the library to dynamically re-compute
a minimum cut on the following graphs:


Orignal Graph 


		        SOURCE
		       /       \
		    78/         \0
		     /      78   \
		   node0 -----> node1
		     |   <-----   |
		     |      0     |
		     \            /
		     0\          /78
		       \        /
		          SINK



Modified Graphs

			  SOURCE
		       /       \
		    15/         \0
		     /     1     \
		   node0 -----> node1
		     |   <-----   |
		     |     4      |
		     \            /
		     5\          /78
		       \        /
		          SINK


Modified Graphs


		        SOURCE
		       /       \
		    78/         \0
		     /     1     \
		   node0 -----> node1
		     |   <-----   |
		     |     4      |
		     \            /
		     0\          /78
		       \        /
		          SINK

		      


///////////////////////////////////////////////////


/* example.cpp */

#include <stdio.h>
#include "graph.h"

int main()
{
	typedef Graph<int,int,int> GraphType;
	GraphType *g = new GraphType(/*estimated # of nodes*/ 2, /*estimated # of edges*/ 1); 

	g -> add_node(); 
	g -> add_node(); 

	g -> edit_tweights(0, 78, 0);
	g -> edit_tweights(1, 0, 78);
	g -> add_edge(0, 1, 78, 0);

	int flow = g -> maxflow();

	printf("Flow = %d\n", flow);
	printf("Minimum cut:\n");
	
	if (g->what_segment(0) == GraphType::SOURCE) printf("node0 is in the SOURCE set\n");
	else printf("node0 is in the SINK set\n");
	
	if (g->what_segment(1) == GraphType::SOURCE) printf("node1 is in the SOURCE set\n");
	else printf("node1 is in the SINK set\n");


	g -> edit_tweights(0, 15, 5);
	/* Re-computing max-flow */
	flow = g -> maxflow(true);

	/* Printing the results */
	printf("Flow = %d\n", flow);
	printf("Minimum cut:\n");
	
	if (g->what_segment(0) == GraphType::SOURCE) printf("node0 is in the SOURCE set\n");
	else printf("node0 is in the SINK set\n");
	if (g->what_segment(1) == GraphType::SOURCE) printf("node1 is in the SOURCE set\n");
	else printf("node1 is in the SINK set\n");

	g -> edit_edge(0,1,1,4);
	/* Re-computing max-flow */
	flow = g -> maxflow(true);

	/* Printing the results */
	printf("Flow = %d\n", flow);
	printf("Minimum cut:\n");
	
	if (g->what_segment(0) == GraphType::SOURCE) printf("node0 is in the SOURCE set\n");
	else printf("node0 is in the SINK set\n");
	if (g->what_segment(1) == GraphType::SOURCE) printf("node1 is in the SOURCE set\n");
	else printf("node1 is in the SINK set\n");


	delete g;
	return 0;
}
