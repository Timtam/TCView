#pragma once

#include <string>
#include <vector>

class BaseProperty
{
  public:
    std::string section;
    std::string key;
    BaseProperty(std::string section, std::string key): section(section), key(key) { }
    virtual std::string get() = 0;
    virtual void set(std::string) = 0;
};

template <class T>
class Property: public BaseProperty
{
  private:
    T value;
  public:
    Property(std::string section, std::string key, T value): BaseProperty(section, key), value(value) { }
    T& operator = (const T &v)
    {
      return this->value = v;
    }
    operator T() const
    {
      return this->value;
    }
    std::string get();
    void set(std::string);
};

class Configuration
{
  private:
    std::vector<BaseProperty*> properties;
    inline void register_property(BaseProperty *p);
  public:
    Property<bool> continuous{"playback", "continuous", true};
    Property<bool> looping{"playback", "looping", true};
    static Configuration * instance();
    void set_file(std::string filename);
  private:
    std::string filename;
    static Configuration *_instance;
    Configuration ();
    Configuration ( const Configuration& );
    ~Configuration ();
    void write();
    class CGuard
    {
      public:
        ~CGuard();
    };
};
