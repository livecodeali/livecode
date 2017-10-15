{
	'variables':
	{
		'javac_wrapper_path': '../tools/javac_wrapper.sh',
		'java_sdk_path': '<!(echo ${JAVA_SDK})',
		'javac_path': '<(java_sdk_path)/bin/javac',
		'jar_path': '<(java_sdk_path)/bin/jar',
	},
	
	'rules':
	[		
		{
			'rule_name': 'javac',
			'extension': 'java',

			'message': '  JAVAC <(RULE_INPUT_DIRNAME)/<(RULE_INPUT_ROOT).class',

			'outputs':
			[
				# Java writes the output file based on the class name.
				# Use some Make nastiness to correct the output name
				'<(PRODUCT_DIR)/>(java_classes_dir_name)/$(subst\t../livecode/engine/,,$(subst\t<(INTERMEDIATE_DIR)/,,$(subst\tsrc/java/,,<(RULE_INPUT_DIRNAME))))/<(RULE_INPUT_ROOT).class',
			],

			'action':
			[
				'<(javac_wrapper_path)',
				'<(javac_path)',
				'1.5',
				'-d', '<(PRODUCT_DIR)/>(java_classes_dir_name)',
				'-implicit:none',
				'<(RULE_INPUT_PATH)',
			],
		},
	],
}
