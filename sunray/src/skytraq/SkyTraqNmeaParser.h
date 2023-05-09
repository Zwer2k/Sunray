// SkyTraqNmeaParser Library
// Copyright (C)2016 SkyTraq Technology, Inc. All right reserved
// web: http://www.navspark.com.tw/
//
// This program is a demo of most of the functions
// in the library.
//
// This program requires a NavSpark GPS/GNSS module.
//
//

#pragma once

typedef signed   char             TS08;
typedef unsigned char             TU08;
typedef signed   short int        TS16;
typedef unsigned short int        TU16;
typedef signed   long int         TS32;
typedef unsigned long int         TU32;
typedef float                     TF32;
typedef double                    TD64;

#define _SUPPORT_GPS_SATELLITES_        0
#define _SUPPORT_GLONASS_SATELLITES_    0
#define _SUPPORT_BEIDOU_SATELLITES_     0


enum ParsingType
  {
    None = 0,
    MessageGGA,
    MessageRMC,
    MessageGLL,
    MessageZDA,
    MessageGNS,
    MessageVTG,

    MessageGNGSA,
    MessageGPGSA,
    MessageGLGSA,
    MessageBDGSA,

    MessageGPGSV,
    MessageGLGSV,
    MessageBDGSV,

    MessagePSTI30,
    MessagePSTI32,

    BufferOverflow,
    MessageUnknown,
  };

enum GnssSystem
{
  GsUnknown,
  GsGps,
  GsGlonass,
  GsBeidou,
  GsGalileo,
};

struct SatelliteInfo
{ 
  SatelliteInfo()
  {
    Clear();
  }
  void Clear()
  {
    sv = 0;
    azimuth = 0;
    elevation = 0;
    cno = 0;
    isInUse = false;
  }
  TU16 sv;
  TU16 azimuth;
  TU16 elevation;
  TU16 cno;
  bool isInUse;
};

class GnssData
{
public:
  GnssData();
  ~GnssData();

public:
   friend class SkyTraqNmeaParser;
  enum QualityMode {
    QM_NotFix,           //RMC 'N', GGA '0'
    QM_Autonomous,       //RMC 'A', GGA '1'
    QM_Differential,     //RMC 'D', GGA '2'
    QM_Precise,          //RMC 'P', GGA '3'
    QM_RealTimeKinematic,//RMC 'R', GGA '4'
    QM_FloatRtk,         //RMC 'F', GGA '5'
    QM_Estimated,        //RMC 'E', GGA '6'
    QM_ManualInput,      //RMC 'M', GGA '7'
    QM_Simulator,        //RMC 'S', GGA '8'
  };

  enum NavigationMode {
    NM_NotFix,
    NM_2DFix,
    NM_3DFix,
  };

public:
  static const double KnotToKmHr() { return 1.852; }
  static const double KnotToMph() { return 1.151; }
  void ClearData() { Init(); }

  //Getter functions
  //Date and Time
  TU16 GetYear() const { return year; }
  TU16 GetMonth() const { return month; }
  TU16 GetDay() const { return day; }
  TU16 GetHour() const { return hour; }
  TU16 GetMinute() const { return minute; }
  TD64 GetSecond() const { return second; }

  //Geographic Information
  TD64 GetLatitude() const { return latitude; }
  TD64 GetLongitude() const { return longitude; }
  TD64 GetAltitudeInMeter() const { return altitudeAboutMeanSeaLevel + geoidalSeparation; }
  TD64 GetAltitudeAboutMeanSeaLevelInMeter() const { return altitudeAboutMeanSeaLevel; }
  TD64 GetGeoidalSeparationInMeter() const { return geoidalSeparation; }

  //Speed and Orientation
  TD64 GetCourseInDegree() const { return courseOverGround; }
  TD64 GetSpeedInKnots() const { return speedKnot; }
  TD64 GetSpeedInKmHr() const { return speedKnot * KnotToKmHr(); }
  TD64 GetSpeedInMph() const { return speedKnot * KnotToMph(); }

  //Accuracy
  TD64 GetHdop() const { return hdop; }
  TD64 GetPdop() const { return pdop; }
  TD64 GetVdop() const { return vdop; }

  //Fix Mode
  QualityMode GetQualitMode() const { return qualityMode; }
  NavigationMode GetNavigationMode() const { return navigationMode; }
  bool IsFix() const { return qualityMode != QM_NotFix;  }
  bool Is2DFix() const { return navigationMode == NM_2DFix;  }
  bool Is3DFix() const { return navigationMode == NM_3DFix;  }

  //Satellites information
  static int GetMaxSatelliteNum() { return MaxSatelliteNum; };
  TU16 GetNumberOfSv() const { return numSV; };
#if (_SUPPORT_GPS_SATELLITES_)
  const SatelliteInfo* GetGpsSatellites() const { return gpSatellites; };
#endif
#if (_SUPPORT_GLONASS_SATELLITES_)
  const SatelliteInfo* GetGlonassSatellites() const { return glSatellites; };
#endif
#if (_SUPPORT_BEIDOU_SATELLITES_)
  const SatelliteInfo* GetBeidouSatellites() const { return bdSatellites; };
#endif
  //RTK information
  TD64 GetEVelocity() const { return eVelocity; };
  TD64 GetNVelocity() const { return nVelocity; };
  TD64 GetUVelocity() const { return uVelocity; };
  TD64 GetRtkAge() const { return rtkAge; };
  TD64 GetRtkRatio() const { return rtkRatio; };
  TD64 GetEProjection() const { return eProjection; };
  TD64 GetNProjection() const { return nProjection; };
  TD64 GetUProjection() const { return uProjection; };
  TD64 GetBaselineLength() const { return baselineLength; };
  TD64 GetBaselineCourse() const { return baselineCourse; };

protected:  //data members
  //Date and Time
  TU16 year;
  TU16 month;
  TU16 day;
  TU16 hour;
  TU16 minute;
  TD64 second;

  //Geographic Information
  TD64 latitude;
  TD64 longitude;
  TD64 altitudeAboutMeanSeaLevel;
  TD64 geoidalSeparation;

  //Speed and Orientation
  TD64 courseOverGround; //Course over ground
  TD64 speedKnot;        //Speed in Knot

  //Accuracy
  TD64 hdop;
  TD64 pdop;
  TD64 vdop;

  //Fix Mode
  QualityMode qualityMode;  //Fix Mode
  NavigationMode navigationMode;

  //Satellites information
  enum { MaxSatelliteNum = 16 };
  TU16 numSV;
#if (_SUPPORT_GPS_SATELLITES_)
  TU16 inUseGpSatellites[MaxSatelliteNum];
  SatelliteInfo gpSatellites[MaxSatelliteNum];
  SatelliteInfo workingGpSatellites[MaxSatelliteNum];
#endif

#if (_SUPPORT_GLONASS_SATELLITES_)
  TU16 inUseGlSatellites[MaxSatelliteNum];
  SatelliteInfo glSatellites[MaxSatelliteNum];
  SatelliteInfo workingGlSatellites[MaxSatelliteNum];
#endif

#if (_SUPPORT_BEIDOU_SATELLITES_)
  TU16 inUseBdSatellites[MaxSatelliteNum];
  SatelliteInfo bdSatellites[MaxSatelliteNum];
  SatelliteInfo workingBdSatellites[MaxSatelliteNum];
#endif
  //RTK information
  TD64 eVelocity; 
  TD64 nVelocity; 
  TD64 uVelocity; 

  TD64 rtkAge;     //Age of differential
  TD64 rtkRatio;   //AR ratio factor for validation

  //ENU-Projection of baseline, meters
  TD64 eProjection; 
  TD64 nProjection; 
  TD64 uProjection; 

  TD64 baselineLength;   //Baseline length, meters
  TD64 baselineCourse;   //Baseline course (angle between baseline vector and north direction), degrees

protected:  //functions
  void Init();
  void CopySatellites(SatelliteInfo* target, const SatelliteInfo* source);
  void ClearSatellites(SatelliteInfo* s);
  void ClearInUseSatellites(TU16* s);

  void AddAnInUsePrn(TU16* s, int p);
  bool AddAnInUsePrnToSatellites(SatelliteInfo* s, int p);
  int FindPrnInSatellites(SatelliteInfo* s, int p, bool addNew) const;
  bool UpdateSatellites(SatelliteInfo* s, int sv, int elv, int az, int cno);
  void UpdateInUseToSatellites(SatelliteInfo* s, const TU16* inUse);

  //Setter functions
  //Date and Time
  bool SetDate(TU16 y, TU16 m, TU16 d);
  bool SetTime(TU16 h, TU16 m, TD64 s);

  //Geographic Information
  bool SetNmeaLatitude(double lat, char ns);
  bool SetNmeaLongitude(double lon, char ew);
  bool SetAltitudeAboutMeanSeaLevel(double alt);
  bool SetGeoidalSeparationInMeter(double gs);

  //Speed and Orientation
  bool SetCourse(TD64 c);
  bool SetSpeedInKnots(TD64 s);

  //Accuracy
  bool SetPdop(TD64 p);
  bool SetHdop(TD64 h);
  bool SetVdop(TD64 v);

  //Fix Mode
  bool SetNavigationMode(NavigationMode m);
  bool SetQualityMode(QualityMode m);

  //Satellites information
  bool SetNumberOfSv(TU16 n);
#if (_SUPPORT_GPS_SATELLITES_)
  bool AddAnInUseGpPrn(int p);
  void ClearInUseGpSatellites();
  bool UpdateGpSatellites(int sv, int elv, int az, int cno);
  void ClearWorkingGpsSatellites();
  void CopyWorkingGpsSatellites(bool reversion = false);
  void UpdateGpsInUseToSatellites();
#endif

#if (_SUPPORT_GLONASS_SATELLITES_)
  bool AddAnInUseGlPrn(int p);
  void ClearInUseGlSatellites();
  bool UpdateGlSatellites(int sv, int elv, int az, int cno);
  void ClearWorkingGlonassSatellites();
  void CopyWorkingGlonassSatellites(bool reversion = false);
  void UpdateGlonassInUseToSatellites();
#endif

#if (_SUPPORT_BEIDOU_SATELLITES_)
  bool AddAnInUseBdPrn(int p);
  void ClearInUseBdSatellites();
  bool UpdateBdSatellites(int sv, int elv, int az, int cno);
  void ClearWorkingBeidouSatellites();
  void CopyWorkingBeidouSatellites(bool reversion = false);
  void UpdateBeidouInUseToSatellites();
#endif

  //RTK information
  bool SetEnuVelocity(double ev, double nv, double uv);
  bool SetRtkAge(TD64 r);
  bool SetRtkRatio(TD64 r);
  bool SetEnuProjection(double ep, double np, double up);
  bool SetBaselineLength(double b);
  bool SetBaselineCourse(double b);
};


class SkyTraqNotifyFun 
{
  public:
    virtual bool gnssUpdated(TU32 f, const char* buf, ParsingType parsingType) = 0;
};


class SkyTraqNmeaParser
{
public: //constructor and destructor
  SkyTraqNmeaParser(void);
  ~SkyTraqNmeaParser(void);

public: //enum and typedef
  
static const TU32 NoUpdate = 0;
static const TU32 UpdateDate            = (0x00000001);
static const TU32 UpdateTime            = (0x00000002);
static const TU32 UpdateLatitude        = (0x00000004);
static const TU32 UpdateLongitude       = (0x00000008);
static const TU32 UpdateAltitude        = (0x00000010);
static const TU32 UpdateCourse          = (0x00000020);
static const TU32 UpdateSpeed           = (0x00000040);
static const TU32 UpdateQualitMode      = (0x00000080);
static const TU32 UpdateNumberOfSv      = (0x00000100);
static const TU32 UpdateHdop            = (0x00000200);
static const TU32 UpdatePdop            = (0x00000400);
static const TU32 UpdateVdop            = (0x00000800);
static const TU32 UpdateNavigationMode  = (0x00001000);
static const TU32 UpdateSatelliteInfo   = (0x00002000);
static const TU32 UpdateEnuVelocity     = (0x00004000);
static const TU32 UpdateRtkAge          = (0x00008000);
static const TU32 UpdateRtkRatio        = (0x00010000);
static const TU32 UpdateEnuProjection   = (0x00020000);
static const TU32 UpdateBaselineLength  = (0x00040000);
static const TU32 UpdateBaselineCourse  = (0x00080000);
  //Notification Callback function type
  //typedef bool (*NotifyFun)(TU32, const char*, ParsingType);

public:   //interface
  ParsingType Encode(TU08 b);
  //Provide a pointer to receive notifications
  void SetNotify(SkyTraqNotifyFun *f) { notifyFunction = f; }
  const GnssData* GetGnssData() const { return &gnssData; }
  const TU08* GetParsingBuffer() const { return buffer; }

protected:  //protected data member
  //Notification callback
  SkyTraqNotifyFun *notifyFunction; 

  GnssData gnssData;
  TU32 updateFlag;
  ParsingType parsingType;

  enum { LineBufferSize = 128 };
  TU08 buffer[LineBufferSize];
  int bufferIndex;

  enum { MaxParamNum = 20 };
  int commaPos[MaxParamNum];
  int commaNum;

protected:  //protected functions
  void Notify();

  void EmptyBuffer();
  void EmptyCommaPos();
  ParsingType ParsingMessage();
  void ScanCommaPos(const TU08* pt, int len);
  ParsingType MessageType(TU08* pt, int size);
  int StrHeaderCompare(const TU08* pt, const TU08* header, int len) const;
  bool VarifyNmeaChecksum(TU08* pt, int len) const;
  int ConvertChecksum(char h, char l) const;
  int HexChar2Int(char c) const;
  GnssData::QualityMode GetGgaQualityMode(char q);
  GnssData::QualityMode GetRmcQualityMode(char q);
  GnssData::NavigationMode GetGsaNavigationMode(char n);

  GnssSystem GetGNSSSystem(int prn);
  ParsingType MessageType(const TU08* pt, int len);
  
  ParsingType ParseNMEA(const TU08* pt, int len);

  void ProcessingGGA(const TU08* pt, int len);
  void ProcessingGSA(GnssSystem gs, const TU08* pt, int len);
  void ProcessingGSV(GnssSystem gs, const TU08* pt, int len);
  void ProcessingRMC(const TU08* pt, int len);
  void ProcessingGLL(const TU08* pt, int len);
  void ProcessingZDA(const TU08* pt, int len);
  void ProcessingVTG(const TU08* pt, int len);
  void ProcessingPSTI30(const TU08* pt, int len);
  void ProcessingPSTI32(const TU08* pt, int len);
};


