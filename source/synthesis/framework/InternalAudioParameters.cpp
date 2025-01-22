#include "InternalAudioParameters.h"
AudioProcessorParameterWithID::AudioProcessorParameterWithID (const std::string& idToUse,
    const String& nameToUse,
    const AudioProcessorParameterWithIDAttributes& attributes)
    : HostedAudioProcessorParameter (idToUse.getVersionHint()),
      paramID (idToUse.getParamID()),
      name (nameToUse),
      label (attributes.getLabel()),
      category (attributes.getCategory()),
      meta (attributes.getMeta()),
      automatable (attributes.getAutomatable()),
      inverted (attributes.getInverted())
{
}

String AudioProcessorParameterWithID::getName (int maximumStringLength) const        { return name.substring (0, maximumStringLength); }
String AudioProcessorParameterWithID::getLabel() const                               { return label; }
AudioProcessorParameter::Category AudioProcessorParameterWithID::getCategory() const { return category; }

