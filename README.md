# Assignment 3
<p> Author: Rand Halasa </p>
<p> Date: Sunday, May 24th, 2024. 11:59 pm </p>

## Software Organisation
<p> The purpose of this software is to implement a P2P File-Transfer program that will allow sending, receiving, and deleting files anf detecting anomolous data chunks. To implement this program, we had to break it down into two parts. </p>

### Part 1
<p> The first part focused on reading a .bpkg file and putting it into a merkle tree format. Each .bpkg file has the following fields: ident, filename, size, nhashes, hashes, nchunks, chunks. </p>
<p> First, the program loads the .bpkg file into a bpkg_obj struct. After running successfully, the bpkg_obj struct will be passed into the initilise_tree(bpkg_obj obj) function to be turned into an array representation of a merkle tree in level order traversal. </p>
<p> To do this, the initilise_tree functin traverses through the data file and breaksit into chunks as specified by the .bpkg file. The merkle tree then computes the hash of each chunk using the sha256 file. Each merkle tree node saves the left and right nodes, if it's a leaf or not, and the expected and computed hash. </p>
<p> Other functions in the first part are implemented to help traverse through the hashes. For example, min_hashes returns the uppermost hash where the expected and computed hashes match. file_check ensures that the data file exists. get_completed_chunks returns all the chunks where expected and computed match, etc. More information on these functions is found in the comments in pkgchk.c. </p>

### Part 2
<p> Part two uses aspects from part 1 to implement the file-to-file transfer. </p>
<p> The first step is to load the configuration file passed in as an argument, since it holds information about the port number, directory, and max number of peers. The config file information is stored in a config_file struct. </p>
<p> After this, a CLI waits for the user to input what they want to do. The options are: </p>
<li>CONNECT</li>
<li>DISCONNECT</li>
<li>ADDPACKAGE</li>
<li>REMPACKAGE</li>
<li>PACKAGES</li>
<li>PEERS</li>
<li>FETCH (not implemented)</li>
<br>
<p> The user would input what they want to run with the appropriate arguments. This will cll on each respective function, decided by the process_input function. </p>
<p> CONNECT would connect a peer to a server, while DISCONNECT would disconnect the peer. The connection is based on the ip address and port number. </p>
<p> ADDPACKAGE and REMPACKAGE add a package to a list and remove it respectively. </p>
<p> PEERS and PACKAGES print out all currently available peers and packages respectively. </p>
<p> A linked list implementation was used to keep track of all the peers and packages. The min hashes algorithm, load bpkg, initilise tree, and other functions from part one were used to facilitate coding part 2. </p>

## Testing
<p> Test are important to ensure that a program runs as expected when presented with expected and unexpected inputs. Tests for this program have been broken down to two parts alligning with the program division </p>

### Testing Part 1
<p> To run the tests for part 1, run make p1tests. </p>
<p> Tests for part one test how the program reacts with incomplete bpkg files, incomplete data files, and expected input. They also check how the merkle tree would work given some of these inputs. </p>

### Testing Part 2
<p> To run the tests for part 1, run make p1tests. </p>
<p> Tests for part one test how the program reacts with incomplete bpkg files, incomplete data files, and expected input. They also check how the merkle tree would work given some of these inputs. </p>
