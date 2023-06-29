from astral import zoneinfo, LocationInfo
from astral.sun import sun

# set location
city = LocationInfo("Rome", "Italy", "Europe/Rome", 41.9028, 12.4964)
timezone = zoneinfo.ZoneInfo("Europe/London")

import datetime
import calendar

output_file = open("daily_sunset.h", "w")
output_file.write(f"""// THIS FILE IS GENERATED, USE "sunsetGenerator.py" TO UPDATE
""")


output_file.write((
    f"// Information for {city.name}/{city.region}\n"
    f"// Timezone: {city.timezone}\n"
    f"// Latitude: {city.latitude:.02f}; Longitude: {city.longitude:.02f}\n\n"
))


#set start date for data
start_time = datetime.datetime(2023,1,1,hour=0, minute=0)
# set sun 


# Number of days of data needed from start date
K = 3650
#creates the time stamps for the range of days defined by K
res = [start_time + datetime.timedelta(days=idx) for idx in range(K)]

output_file.write(f"""
// This data was generated on {start_time}
// It contains nearly 10 years of sunsets

// The variable below makes it possible for the Teensy
// to determine offset to the current day in the data 
""")

output_file.write(f"time_t sunset_data_start_date = {calendar.timegm(start_time.timetuple())};\n\n")

output_file.write("""
const uint32_t sunsetdate[] = {\n""")
for n, d in enumerate(res):
    sunset = sun(city.observer, date=d,  tzinfo=timezone)['sunset'] # get sunset from sun
    sunset_e = calendar.timegm(sunset.timetuple()) # get date as epoch seconds
    
    # If loop makes sure the last variable has no ","
    if n < K-1:   output_file.write(f"{sunset_e}, //{n} : {sunset}\n")
    else:
        output_file.write(f"{sunset_e} //{n} : {sunset}\n")
        print(n)
    
output_file.write("};\n")
output_file.close()
    
print("end")


output_file = open("daily_schedule.h", "w")
output_file.write(f"""// THIS FILE IS GENERATED, USE ASTRALGENERATOR.PY TO UPDATE
""")

start_time = datetime.datetime(1970,1,1,hour=00, minute=0)
performaces = 24
minutes_gap = 60
res = [start_time + datetime.timedelta(minutes=idx*minutes_gap) for idx in range(performaces)]

output_file.write(f"""
uint8_t next_perfomance_number = 0;

// This is the times that performaces will happen during the day
uint32_t daily_schedule[{performaces}] = {{
"""
                  )

for n,x in enumerate(res):
    print(x)
    if n < performaces-1: output_file.write(f"{calendar.timegm(x.timetuple())}, // {x.time()}\n")
    else:
        output_file.write(f"{calendar.timegm(x.timetuple())} // {x.time()} \n}};")
    
output_file.close()
    
print("end")