#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include "mex.h"

using namespace std;

typedef struct Coordinate
{
    double time;
    double lat;
    double lon;
    double alt;
} Coordinate;

class KMLGenerator
{
private:
    // Full filepath to the log file to be parsed
    string filepath;

    // Name of the log file (without extension and the path to the folder
    // containing it
    string dat_file_name;
    string dat_file_folder;

    string kml_file_name;
    string kml_file_folder;

    string kml_name;
    string kml_path_name;
    string linestyle_color = "ff00aaff";
    string linestyle_width = "5";

    // Input file stream to read the file
    ifstream file;
    const string latitude_labels[2] = {"lat", "latitude"};
    const string longitude_labels[3] = {"lon", "long", "longitude"};
    const string altitude_labels[2] = {"alt", "altitude"};
    const string depth_labels[2] = {"depth", "dpeth"};
    int latitude_size = sizeof(latitude_labels) / sizeof(latitude_labels[0]);
    int longitude_size = sizeof(longitude_labels) / sizeof(longitude_labels[0]);
    int altitude_size = sizeof(altitude_labels) / sizeof(altitude_labels[0]);
    int depth_size = sizeof(depth_labels) / sizeof(depth_labels[0]);

    // LogFile struct to contain the log file's data
    vector<Coordinate> coordinate_data;
    string kml_string = "";

public:
    KMLGenerator(string filepath) : filepath(filepath)
    {
        char folder_delimiter = '/';

#ifdef _WIN32
        folder_delimiter = '\\';
#endif

        // Parse the name of the log file from the filepath

        // Locate the last instance of the folder delimiter characters,
        // and take all characters after it to get the log file name with
        // the file extension
        size_t last_delimiter_pos = filepath.find_last_of(folder_delimiter);
        dat_file_name = filepath.substr(last_delimiter_pos + 1, string::npos);
        dat_file_folder = filepath.substr(0, last_delimiter_pos + 1);

        kml_file_folder = dat_file_folder.substr(0, dat_file_folder.size() - 4);
        kml_file_name = dat_file_name.substr(0, dat_file_name.size() - 4);
        kml_file_name = kml_file_name.append(".kml");

        string answer;
        for (int i = 0; i < filepath.length(); i++)
        {
            answer = filepath.substr(i, 19);
            if (is_time_formatted_correctly(answer))
            {
                break;
            }
        }

        kml_name = answer + kml_file_name;
        kml_path_name = kml_file_name;

        // Open the log file to be read
        file.open(filepath);
        if (!file.is_open())
        {
            mexPrintf("Failed to open file %s\r\n", filepath.c_str());
            mexCallMATLAB(0, NULL, 0, NULL, "drawnow");
        }
    }

    void generate_kml_file()
    {
        // Create a file for the custom tag
        string output_filepath = kml_file_folder + kml_file_name;
        ofstream output_file(output_filepath);
        mexPrintf("    Writing to file %s \n", output_filepath.c_str());
        read_dat_file();
        mexCallMATLAB(0, NULL, 0, NULL, "drawnow");

        if (output_file.is_open())
        {
            generate_kml_string();
            output_file << kml_string << endl;
            output_file.close();
        }
        else
        {
            mexPrintf("    Failed to open output file %s \n", output_filepath.c_str());
            mexCallMATLAB(0, NULL, 0, NULL, "drawnow");
        }
    }

private:
    void generate_kml_header()
    {
        kml_string.append("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
        kml_string.append("<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\" xmlns:kml=\"http://www.opengis.net/kml/2.2\" xmlns:atom=\"http://www.w3.org/2005/Atom\">\n");
        kml_string.append("<Document>\n");
    }

    bool is_time_formatted_correctly(string time)
    {
        int number_position[] = {0, 1, 2, 3, 5, 6, 8, 9, 11, 12, 14, 15, 17, 18};
        int dash_position[] = {4, 7};
        int dot_position[] = {13, 16};
        int underscore_position[] = {10};

        if (time.length() != 19)
        {
            return false;
        }
        else
        {
            for (auto &i : number_position)
            {
                if (!isdigit(time.at(i)))
                {
                    return false;
                }
            }

            for (auto &i : dash_position)
            {
                if (time.at(i) != '-')
                {
                    return false;
                }
            }

            for (auto &i : dot_position)
            {
                if (time.at(i) != '.')
                {
                    return false;
                }
            }

            if (time.at(10) != '_')
            {
                return false;
            }
        }
        return true;
    }

    void generate_kml_style()
    {
        string name_string = "<name>" + kml_name + "</name>\n";
        kml_string.append(name_string);
        kml_string.append("<Style id=\"test\">\n");
        kml_string.append("<LineStyle>\n");
        string color_string = "<color>" + linestyle_color + "</color>\n";
        kml_string.append(color_string);
        string width_string = "<width>" + linestyle_width + "</width>\n";
        kml_string.append(width_string);
        kml_string.append("</LineStyle>\n");
        kml_string.append("</Style>\n");
    }

    void generate_kml_header_2()
    {
        kml_string.append("<Placemark>\n");
        string name_string = "<name>" + kml_path_name + "</name>\n";
        kml_string.append(name_string);
        kml_string.append("<styleUrl>#test</styleUrl>\n");
        kml_string.append("<LineString>\n");
        kml_string.append("<tessellate>1</tessellate>\n");
        kml_string.append("<coordinates>\n");
    }

    void generate_kml_data()
    {
        int counter = 0;
        for (auto &coord : coordinate_data)
        {
            if ((abs(coord.lon) < 0.0001) && (abs(coord.lat) < 0.0001)) {
                // skip
            } else if ((to_string(coord.lon) != "nan") && (to_string(coord.lat) != "nan") && (to_string(coord.alt) != "nan") && (counter >= 10)) {
                string temp = to_string(coord.lon) + "," + to_string(coord.lat) + "," + to_string(coord.alt) + "\n";
                kml_string.append(temp);
                counter = 0;
            } else if ((to_string(coord.lon) != "nan") && (to_string(coord.lat) != "nan") && (to_string(coord.alt) != "nan") && (counter < 10)) {
                counter++;
            }
        }
    }

    void generate_kml_footer()
    {
        kml_string.append("</coordinates>\n");
        kml_string.append("</LineString>\n");
        kml_string.append("</Placemark>\n");
        kml_string.append("</Document>\n");
        kml_string.append("</kml>\n");
    }

    void generate_kml_string()
    {
        generate_kml_header();
        generate_kml_style();
        generate_kml_header_2();
        generate_kml_data();
        generate_kml_footer();
    }

    vector<string> split(const string &str, const string &delim)
    {

        vector<string> substrings;
        size_t pos = 0;
        size_t prev_pos = 0;

        // Loop through the string looking for the delimeter
        do
        {

            pos = str.find(delim, prev_pos);
            if (pos == string::npos)
            {
                pos = str.length();
            }

            // Get the substring between the previous delimeter and the new one
            string token = str.substr(prev_pos, pos - prev_pos);
            substrings.push_back(token);

            prev_pos = pos + delim.length();

        } while (pos < str.length() && prev_pos < str.length());

        return substrings;
    }

    string strip(string str, char character)
    {
        string output;
        for (size_t i = 0; i < str.size(); i++)
            if (str[i] != character)
                output += str[i];
        return output;
    }

    bool has_element(vector<string> vec, string element)
    {
        bool matches = false;
        for (size_t i = 0; i < vec.size(); i++)
            if (!element.compare(vec.at(i)))
                matches = true;
        return matches;
    }

    int find_element(vector<string> vec, string element)
    {
        for (size_t i = 0; i < vec.size(); i++)
            if (!element.compare(vec.at(i)))
                return i;
        return -1;
    }

    int find_element_from_array(vector<string> vec, const string *element, int size)
    {
        for (int i = 0; i < size; i++)
        {
            string x = element[i];
            if (find_element(vec, x) != -1)
            {
                return find_element(vec, x);
            }
        }
        return -1;
    }

    int *find_lat_lon_alt_index(string line)
    {
        vector<string> line_vector = split(line, " ");

        int lat_index = find_element_from_array(line_vector, latitude_labels, latitude_size);
        int lon_index = find_element_from_array(line_vector, longitude_labels, longitude_size);
        int alt_index = find_element_from_array(line_vector, altitude_labels, altitude_size);

        static int answer[3];
        answer[0] = lat_index;
        answer[1] = lon_index;
        answer[2] = alt_index;

        for (int i : answer)
        {
            if (i == -1)
            {
                mexErrMsgTxt("Latitude, longitude, or altitude was not found in the dat file");
            }
        }

        return answer;
    }

    void read_dat_file()
    {
        string line;
        getline(file, line);
        int *lat_lon_alt_index = find_lat_lon_alt_index(line);

        // Read lines from the file until no lines remain
        while (getline(file, line))
        {
            try
            {

                // Remove any newline or carraige return characters from the line
                //line = strip(line, '\r');
                //line = strip(line, '\n');

                // If the last tag index is npos, no ] character was found, and
                // the line does not contain any tags. This line is therefore
                // invalid and can be ignored
                if (line == "")
                {
                    mexPrintf("    Ignoring invalid log line: %s \n", line.c_str());
                    continue;
                }
                vector<string> line_tags = split(line, " ");

                Coordinate coordinate;
                coordinate.time = stod(line_tags.at(0));
                coordinate.lat = stod(line_tags.at(lat_lon_alt_index[0]));
                coordinate.lon = stod(line_tags.at(lat_lon_alt_index[1]));
                coordinate.alt = stod(line_tags.at(lat_lon_alt_index[2]));

                coordinate_data.push_back(coordinate);
            }
            catch (const exception &ex)
            {
                mexPrintf("    Ignoring invalid log line: %s \n", line.c_str());
                mexCallMATLAB(0, NULL, 0, NULL, "drawnow");
            }
        }
        // Close the file since we're done reading from it
        file.close();

        mexPrintf("    Successfully read file \n");
    }
};

void mexFunction(int nlhs, mxArray *plhs[],
                 int nrhs, const mxArray *prhs[])
{

    // Check for proper number of input and output arguments
    if (nrhs != 1)
        mexErrMsgIdAndTxt("MATLAB:revord:invalidNumInputs", "One input required.");
    else if (nlhs > 1)
        mexErrMsgIdAndTxt("MATLAB:revord:maxlhs", "Too many output arguments.");

    // Input argument must be a string
    if (mxIsChar(prhs[0]) != 1)
        mexErrMsgIdAndTxt("MATLAB:revord:inputNotString", "Input 1 must be a string.");

    // Get the input argument as a C string
    char *filepath_buf = mxArrayToString(prhs[0]);

    // Ensure that the input argument was converted to a string properly
    if (filepath_buf == NULL)
        mexErrMsgIdAndTxt("MATLAB:revord:conversionFailed", "Could not convert input to string.");

    // Create a C++ string from the C string
    string filepath(filepath_buf);
    mxFree(filepath_buf);

    // Run the log parser
    mexPrintf("\n================================================================================\n\n");
    mexPrintf("Making KML file %s \n", filepath.c_str());
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow");
    KMLGenerator generator(filepath);
    generator.generate_kml_file();
    mexPrintf("Finished making KML file %s \n", filepath.c_str());
    mexCallMATLAB(0, NULL, 0, NULL, "drawnow");

    return;
}