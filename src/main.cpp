#include <ctime>
#include <climits>
#include <unistd.h>
#include <iostream>
#include <sys/stat.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <sstream>

using namespace std::string_literals;

std::string getHostname()
{
    char hostname[HOST_NAME_MAX + 1];
    gethostname(hostname, HOST_NAME_MAX + 1);
    std::string hs(hostname);
    return hs;
}

/**
 * @brief Get current date/time in format %d-%m_%H:%M
 *
 * @return string: date in %d-%m_%H:%M format
 */
std::string getBackupTime()
{
    time_t now = time(nullptr);
    struct tm tstruct = *localtime(&now);
    char buf[80];

    strftime(buf, sizeof(buf), "%d-%m_%H:%M", &tstruct);
    return buf;
}

/**
 * @brief Gets the Time From File Attribute
 *
 * @param path of file to read attributes from
 * @return string: time in %H:%M format
 */
std::string getTimeFromFileAttribute(const std::string &path)
{
    struct stat fileInfo;
    if (stat(path.c_str(), &fileInfo) != 0)
    {
        perror("Error of getting files' attributes");
        exit(1);
    }
    struct tm tstruct = *localtime(&fileInfo.st_ctime);
    char buf[80];

    strftime(buf, sizeof(buf), "%H:%M", &tstruct);
    return buf;
}

/**
 * @brief Checks if a directory exists
 *
 * @param path of a directory
 * @return true if exists
 * @return else false
 */
bool directoryExists(const std::string &path)
{
    struct stat info;
    return (stat(path.c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
}

/**
 * @brief Creates a Directory to store backups
 *
 * @param path where to create directory
 * @return true if directory is created
 * @return else false
 */
bool createDirectory(const std::string &path)
{
    auto command = "mkdir " + path;
    if (system(command.c_str()) != 0)
    {
        perror("mkdir");
        return false;
    }
    return true;
}

/**
 * @brief Creates a Backup Directory
 *
 * @param hostPath
 * @return string: Backeup Directory path
 */
std::string createBackupDirectory(const std::string &hostPath)
{
    auto directoryName = getBackupTime();
    std::string archiveDirectory = hostPath + "/" + directoryName;

    // Check if the target directory already exists
    if (directoryExists(archiveDirectory))
    {
        std::cout << "Directory with a similar name already exists. Exiting program." << std::endl;
        exit(1); // Exit with an error code
    }
    createDirectory(archiveDirectory);
    return archiveDirectory;
}

/**
 * @brief Copies files in the path to archiveDirectory
 *
 * @param hostname
 * @param origin
 * @param destination
 */
void makeBackup(
    const std::string &hostname,
    const std::string &origin,
    const std::string &destination)
{
    // Construct the command to copy the home directory
    std::string command = "rsync --exclude '" + hostname + "' --recursive --progress -a " + origin + " " + destination;

    // Execute the command
    std::cout << "Copying is started..." << std::endl;
    int result = system(command.c_str());

    if (result != 0)
    {
        std::cout << "Failed to copy the home directory." << std::endl;
        exit(1); // Exit with an error code
    }
    std::cout << "Home directory copied successfully to: " << destination << std::endl;
}

/**
 * @brief Archives files using zip linux command.
 *
 * @param archiveName string: path to a directory (without .zip extension) to save a zip file.
 * @param filesToAcrhive vector<string>: list of files to add to a zip file.
 */
std::string archiveFilesWithZip(std::string archiveName,
                                const std::vector<std::string> &filesToArchive)
{
    // Make a string of files to archive
    std::ostringstream archiveStream;
    for (const auto &file : filesToArchive)
    {
        archiveStream << file << " ";
    }

    archiveName = archiveName + ".zip"s;
    std::string command = "zip -rvy -9 "s + archiveName + " " + archiveStream.str();

    int result = system(command.c_str());
    if (result != 0)
    {
        std::cout << "Failed to create a zip file." << std::endl;
        exit(1); // Exit with an error code
    }

    return archiveName;
}

void deleteDirectory(const std::string &path)
{
    std::string command = "rm -vrdf " + path;
    int result = system(command.c_str());
    if (result != 0)
    {
        std::cout << "Failed to delete the " + path << std::endl;
        exit(1); // Exit with an error code
    }
}

int main()
{
    // 1. Get a directory path to copy
    std::cout << "Enter the path where to copy home directory:" << std::endl;
    std::string path;
    std::cin >> path;

    // 2. Create a hostname directory (if not exists)
    const std::string hostname = getHostname();
    auto hostPath = path + "/" + hostname;
    createDirectory(hostPath);

    // 3. Create an archive directory with dd-MM-hh-m name
    auto archiveDirectory = createBackupDirectory(hostPath);

    // 4. Start copying files to the archive directory
    makeBackup(hostname, path, archiveDirectory);

    // 5. Create a zip file with name based on creation attribute of the created directory
    auto archivePath = hostPath + "/"s + getTimeFromFileAttribute(archiveDirectory);
    archiveFilesWithZip(archivePath, {archiveDirectory});

    // 6. Delete created directory
    deleteDirectory(archiveDirectory);
    return 0;
}