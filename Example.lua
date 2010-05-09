local io = require("NS2_IO")

	--checks weather a file/directory exists
	local boolExists = io.Exists("/lua")

	--finds all the lua files in the /lua directory
	local files = io.FindFiles("/lua/*.lua")
	
	for _,file in ipairs(files) do
		Shared.Message(file)
	end
	
	for _,dir in ipairs(io.FindDirectorys("/lua/*")) do
		Shared.Message(dir)
	end
	
	--returns the orded list of root directorys that ns2 will try to find a file not much use for this function yet
	--examples /core/, /ns2/
	for _,dir in ipairs(io.GetDirRootList()) do
		Shared.Message(dir)
	end
	
	--returns the value of /game commandline argument
	Shared.Message(io.GetGameString())
