# Parasort

Comparison of Parallel Sorting Algorithms

## Project overview

The aim of this project is to study the well known problem of sorting _atomic_ items
(i.e. they occupy O(1) space) within the context of parallel computing.
All the standard sequential sorting algorithms can be re-designed to exploit parallel machines;
as a matter of fact, literature is not poor of this kind of works.
There are some sequentialalgorithms (such as mergesort or quicksort) that can be easily adapted
to the new parallel context; nevertheless, other ones needs a trickier re-engineering.
On the other hand, algorithms that performs very well on sequential machines could not be so good
in their new parallel version. Thus, the first objective of this project is to experimentally
compare the performance of the most known parallel sorting algorithms. 

In this scenario things get even more complicated from the fact that we have to manage very large
data sets, i.e. our parallel algorithms sort gigabytes of data.
Therefore, the theoretical results achieved in the field of _external memory sorting_ will be of
fundamental importance. On the other hand, in order to keep limited the implementation complexity,
we have decided to not extend our parallel algorithms for exploiting the presence of multi-disks
(potentially provided by the parallel architecture). 

The architectural features of the test environment become crucial.
Obviously, we cannot say neither that our algorithms will behave the same way on every possible
parallel machine nor that performance results are meaningful just for a specific architecture.
Rather, we will deeply comment the performance results of each algorithm keeping in mind the
architectural limitations of the test environment.
At a first glance, we will have to take care of at least two macroscopic aspects:
first, the possibility that the parallel machine is a hierarchical systems
(e.g. a cluster of shared memory nodes); second, the interconnection network of the nodes.
In case of a hierarchical system, if we will be able to find a way for exploiting the presence
of shared memory, then we might achieve a very significant gain in terms of performance.
Thus, depending both on the way we will exploit shared memory and the properties of the
interconnection structure (bandwidth, latency), we may end up with performance results more
or less close to the ideality.

We will address all this issues implementing parallel sorting algorithms on top of a new
structured framework and analyzing them from both a theoretical and an empirical point of view.
This document adopts a top-down approach: first, we explain which sorting algorithms will be
parallelized; second, we describe the main features of our framework; then, after having described
the test environment, we will analyze the performance of each parallel sorting algorithm.
Finally, we will compare all the achieved results.
