#include "calc.h"

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include <nlohmann/json.hpp>

using std::string;
using json = nlohmann::json;

json make_json_pair(const string& uom, const double& value) {
    return {{"uom", uom}, {"value", value}};
}

bool numeric(string value) {
    auto ss = std::stringstream(value);
    double tmp;
    ss >> tmp;
    return !(!ss || ss.rdbuf()->in_avail() > 0);
}
response_t validate(const kvp& query_params)
{
    response_t response {true, json::object({ {"status","success"} })};
    validate_air_temp(query_params, &response);
    validate_air_temp_uom(query_params, &response);
    validate_dewpoint(query_params, &response);
    validate_dewpoint_uom(query_params, &response);
    validate_relative_humidity(query_params, &response);
    validate_input_values(query_params, &response);
    validate_dew_rel_hum (query_params, &response);

    return response;
}

response_t validate_air_temp (const kvp& query_params, response_t* r) {
    auto it = query_params.find("air_temp");
    if (it != query_params.end() && !it->second.empty()) {
        if(numeric(it->second)) {
            r->input.air_temp = atof(it->second.c_str());
        } else {
            r->valid = false;
            r->doc["status"] = "error";
            r->doc["message"] = "Non-numeric value provided for air_temp.";
            r->doc["expected"] = "a floating point value";
            r->doc["actual"] = it->second;
        }
    } else if (it->second.empty()) {
        r->valid = false;
        r->doc["status"] = "error";
        r->doc["message"] = "No value provided for air_temp input parameter.";
        r->doc["expected"] = "a floating point value >80 deg Fahrenheit";
        r->doc["actual"] = it->second;
    }   else {
        r->valid = false;
        r->doc["status"] = "error";
        r->doc["message"] = "Required input parameter not specified.";
        r->doc["expected"] = "air_temp";
        r->doc["actual"] = nullptr;
    }
}

response_t validate_air_temp_uom(const kvp& query_params, response_t* r) {
    auto it = query_params.find("air_uom");
    if (it != query_params.end()) {
        string uom = it->second;
        uom = toupper(uom[0]);
        if (uom == "F") {
            r->input.air_temp = cvt_f_c(r->input.air_temp);
        } else if (uom == "C") {
            // No action needed
        }
        else {
            r->valid = false;
            r->doc["stats"] = "error";
            r->doc["message"] = "Unknown unit of measure provided.";
            r->doc["expected"] = "One of 'uom=C' or 'uom=F'.";
            r->doc["actual"] = it->second;
          }
    }
}

response_t validate_dewpoint (const kvp& query_params, response_t* r) {
    auto it = query_params.find("dew_temp");
    if (it != query_params.end() && !it->second.empty()) {
        if (numeric(it->second)) {
            r->input.dew_temp = atof(it->second.c_str());
        }   else if (it->second.empty()){
                r->valid = false;
                r->doc["status"] = "error";
                r->doc["message"] = "No value provided for dew_temp input parameter.";
                r->doc["expected"] = "a floating point value [-405.4 F, air_temp]";
                r->doc["actual"] = it->second;
        }
        else {
            r->valid = false;
            r->doc["status"] = "error";
            r->doc["message"] = "Non-numeric value provided for dew_temp.";
            r->doc["expected"] = "a floating point value [-405.4 F, air_temp]";
            r->doc["actual"] = it->second;
        }
    }
}

response_t validate_dewpoint_uom(const kvp& query_params, response_t* r) {
    auto it = query_params.find("dew_uom");
    if (it != query_params.end()) {
        string dew_uom = it->second;
        dew_uom = toupper(dew_uom[0]);
        if (dew_uom == "C") {
           // No action needed
        } else if (dew_uom == "F") {
            r->input.dew_temp = cvt_f_c(r->input.dew_temp);
        }
        else {
            r->valid = false;
            r->doc["status"] = "error";
            r->doc["message"] = "Unknown unit of measure provided.";
            r->doc["expected"] = "One of 'dew_uom=C' or 'dew_uom=F'.";
            r->doc["actual"] = it->second;
        }

    }
}

response_t validate_relative_humidity (const kvp& query_params, response_t* r) {
    auto it = query_params.find("relative_humidity");
    if (it != query_params.end() && !it->second.empty()) {
        if (numeric(it->second)) {
            r->input.relative_humidity = atof(it->second.c_str());
        }   else if (it->second.empty()) {
                r->valid = false;
                r->doc["status"] = "error";
                r->doc["message"] = "No value provided for relative_humidity input parameter.";
                r->doc["expected"] = "a floating point value (0,100)";
                r->doc["actual"] = it->second;
        }
        else {
            r->valid = false;
            r->doc["status"] = "error";
            r->doc["message"] = "Non-numeric value provided for relative_humidity.";
            r->doc["expected"] = "a floating point value (0,100)";
            r->doc["actual"] = it->second;
        }
    }
}

response_t validate_input_values(const kvp& query_params, response_t* r) {
    if (r->input.air_temp < (80/3)) {
        r->valid = false;
        r->doc["status"] = "error";
        r->doc["message"] =
        "The valid input limits for air temperature is greater than 80 deg Fahrenheit or 26.66667 deg Celsius.";
    }
    if (r->input.dew_temp < -243.0 || r->input.dew_temp > r->input.air_temp) {
        r->valid = false;
        r->doc["status"] = "error";
        r->doc["message"] =
        "The valid input limits for dewpoint temperature are between -243C and the input air temperature.";
    }
    if (r->input.relative_humidity < 40.0) {
        r->valid = false;
        r->doc["status"] = "error";
        r->doc["message"] = "The valid input limits for relative humidity is greater than 40.";
    }
}
// Prevents calculation if BOTH relative humidity and dewpoint temperature are input
response_t validate_dew_rel_hum (const kvp& query_params, response_t* r) {
    auto it = query_params.find("relative_humidity");
    auto it2 = query_params.find("dew_temp");
    if (it != query_params.end() && !it->second.empty()) {
        if (it2 != query_params.end() && !it2->second.empty()) {
            r->valid = false;
            r->doc["status"] = "error";
            r->doc["message"] = "Requires rh or dew_temp, not both.";
            r->doc["expected"] = "a floating point value rh = (0,100) or dew_temp = [-405.4F , air_temp]";
            r->doc["actual"] = it->second, it2->second;
        }
    }
}


response_t calculate (const response_t& r) {
    auto r = response;
    response_t cvt_dew_rh(const kvp& query_params, response_t* r) {
        auto it = query_params.find("relative_humidity");
        if (it != query_params.end() && !it ->second.empty()) {
            double v_p = calculate_vapor_pressure(r->input.air_temp);
            double s_v_p = calculate_vapor_pressure(r->input.dew_temp);
            calculate_relative_humidity(v_p, s_v_p);
            return calculate_relative_humidity(v_p, s_v_p);

            r->input.air_temp = cvt_c_f(r->input.air_temp);
            auto heat_index = calculate_heat_index(r->input.air_temp, calculate_relative_humidity(v_p, s_v_p));
            r->doc["data"]["absolute_humidity"] = make_json_pair("deg F", heat_index);
        }
        else {
            r->input.air_temp = cvt_c_f(r->input.air_temp);
            auto heat_index = calculate_heat_index(r->input.air_temp, r->input.relative_humidity);
            r->doc["data"]["absolute_humidity"] = make_json_pair("deg F", heat_index);
        }
    }
    return r;
}


/*




// Determine which path to take
    response_t path (const std::map<std::string, std::string>& query_params) {
        response_t r = {true, {"status","success"}};
        auto it = query_params.find("rh");
        auto it2 = query_params.find("dew_temp");
        if (!it->second.empty() && it2 ->second.empty()) {
            r.input.air_temp = cvt_c_f(r.input.air_temp);
            auto heat_index = calc(r.input.air_temp,r.input.rh);
        }
        else {
            auto rh = relative_humidity(r.input.air_temp, r.input.dew_temp);
            r.input.air_temp = cvt_c_f(r.input.air_temp);
            auto heat_index = calc(r.input.air_temp, rh);
        }
        return r;
    }
*/


