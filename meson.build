project('ProbabilityGuesser', 'c')

# dependencies
curl = dependency('libcurl', required: true)
cjson = dependency('libcjson')

dependencies = [curl, cjson]

src_files = ['ProbabilityGuesser.c', 'data.c', 'predictor.c']

executable('ProbabilityGuesser', src_files, dependencies: dependencies)