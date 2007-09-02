set(raknet_SRC
	
	AsynchronousFileIO.cpp AsynchronousFileIO.h
	AutopatcherPatchContext.h
	AutopatcherRepositoryInterface.h
	BigTypes.h
	BitStream.cpp BitStream.h
	BitStream_NoTemplate.cpp
	CheckSum.cpp CheckSum.h
	ClientContextStruct.h
	CommandParserInterface.cpp CommandParserInterface.h
	ConnectionGraph.cpp ConnectionGraph.h
	ConsoleServer.cpp ConsoleServer.h
	DataBlockEncryptor.cpp DataBlockEncryptor.h
	DataCompressor.cpp DataCompressor.h
	DirectoryDeltaTransfer.cpp DirectoryDeltaTransfer.h
	DS_BinarySearchTree.h
	DS_BPlusTree.h
	DS_BytePool.cpp DS_BytePool.h
	DS_ByteQueue.cpp DS_ByteQueue.h
	DS_Heap.h
	DS_HuffmanEncodingTree.cpp
	DS_HuffmanEncodingTreeFactory.h
	DS_HuffmanEncodingTree.h
	DS_HuffmanEncodingTreeNode.h
	DS_LinkedList.h
	DS_List.h
	DS_Map.h
	DS_MemoryPool.h
	DS_OrderedChannelHeap.h
	DS_OrderedList.h
	DS_Queue.h
	DS_QueueLinkedList.h
	DS_RangeList.h
	DS_Table.cpp DS_Table.h
	DS_Tree.h
	DS_WeightedGraph.h
	EmailSender.cpp EmailSender.h
	EncodeClassName.cpp
	Export.h
	ExtendedOverlappedPool.cpp ExtendedOverlappedPool.h
	FileList.cpp FileList.h
	FileListTransferCBInterface.h
	FileListTransfer.cpp FileListTransfer.h
	FileOperations.cpp FileOperations.h
	_FindFirst.cpp _FindFirst.h
	FullyConnectedMesh.cpp FullyConnectedMesh.h
	GetTime.cpp GetTime.h
	InternalPacket.h
	LightweightDatabaseClient.cpp LightweightDatabaseClient.h
	LightweightDatabaseCommon.cpp LightweightDatabaseCommon.h
	LightweightDatabaseServer.cpp LightweightDatabaseServer.h
	LinuxStrings.cpp LinuxStrings.h
	LogCommandParser.cpp LogCommandParser.h
	MessageFilter.cpp MessageFilter.h
	MessageIdentifiers.h
	MTUSize.h
	NatPunchthrough.cpp NatPunchthrough.h
	NetworkIDManager.cpp NetworkIDManager.h
	NetworkIDObject.cpp NetworkIDObject.h
	PacketConsoleLogger.cpp PacketConsoleLogger.h
	PacketFileLogger.cpp PacketFileLogger.h
	PacketLogger.cpp PacketLogger.h
	PacketPool.h
	PacketPriority.h
	PluginInterface.cpp PluginInterface.h
	RakAssert.h
	RakNetCommandParser.cpp RakNetCommandParser.h
	RakNetDefines.h
	RakNetStatistics.cpp RakNetStatistics.h
	RakNetTransport.cpp RakNetTransport.h
	RakNetTypes.cpp RakNetTypes.h
	RakNetworkFactory.cpp RakNetworkFactory.h
	RakPeer.cpp RakPeer.h
	RakPeerInterface.h
	RakSleep.cpp RakSleep.h
	Rand.cpp Rand.h
	ReliabilityLayer.cpp ReliabilityLayer.h
	ReplicaEnums.h
	Replica.h
	ReplicaManager.cpp ReplicaManager.h
	Rijndael-Boxes.h
	rijndael.cpp
	Rijndael.h
	Router.cpp Router.h
	RouterInterface.h
	RPCMap.cpp RPCMap.h
	RPCNode.h
	RSACrypt.h
	SHA1.cpp SHA1.h
	SimpleMutex.cpp SimpleMutex.h
	SimpleTCPServer.h
	SingleProducerConsumer.h
	SocketLayer.cpp SocketLayer.h
	StringCompressor.cpp StringCompressor.h
	StringTable.cpp StringTable.h
	SystemAddressList.cpp SystemAddressList.h
	TableSerializer.cpp TableSerializer.h
	TCPInterface.cpp TCPInterface.h
	TelnetTransport.cpp TelnetTransport.h
	ThreadPool.h
	ThreadsafePacketLogger.cpp ThreadsafePacketLogger.h
	TransportInterface.h
	)

add_library(raknet STATIC ${raknet_SRC})
