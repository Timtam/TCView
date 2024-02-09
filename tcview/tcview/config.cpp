#include "stdafx.h"
#include "config.h"
#include <stdlib.h>
#include <sstream>

template <class T>
std::string Property<T>::get()
{
  return std::to_string(this->value);
}

template <class T>
void Property<T>::set(std::string v)
{
  std::istringstream s{v};
  s >> this->value;
}

Configuration::Configuration()
{
  this->register_property(&(this->continuous));
  this->register_property(&(this->looping));
  this->register_property(&(this->volume));
}

Configuration * Configuration::instance()
{
  static Configuration::CGuard g;
  if(Configuration::_instance == NULL)
    Configuration::_instance = new Configuration();
  return Configuration::_instance;
}

Configuration::CGuard::~CGuard()
{
  if(Configuration::_instance != NULL)
  {
    delete Configuration::_instance;
    Configuration::_instance = NULL;
  }
}

void Configuration::set_file(std::filesystem::path filename)
{
  std::string value;
  unsigned int i;
  this->filename = filename;
  // reading in data for each property
  for(i=0;i<this->properties.size();i++)
  {
    value.resize(MAX_PATH);
    GetPrivateProfileStringA(this->properties[i]->section.c_str(), this->properties[i]->key.c_str(), this->properties[i]->get().c_str(), &value.front(), MAX_PATH, filename.string().c_str());
    if(GetLastError() == 0x2)
      continue;
    value.resize( strlen(value.data() ));
    this->properties[i]->set(value);
  }
}

void Configuration::register_property(BaseProperty *p)
{
  this->properties.push_back(p);
}

Configuration::~Configuration()
{
  this->write();
}

void Configuration::write()
{
  unsigned int i;
  for(i=0;i<this->properties.size();i++)
    WritePrivateProfileStringA(this->properties[i]->section.c_str(), this->properties[i]->key.c_str(), this->properties[i]->get().c_str(), this->filename.string().c_str());
}

Configuration * Configuration::_instance = NULL;
