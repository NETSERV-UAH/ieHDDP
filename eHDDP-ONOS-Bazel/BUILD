COMPILE_DEPS = CORE_DEPS + KRYO + CLI + [
    "//core/store/serializers:onos-core-serializers",
    "//core/store/primitives:onos-core-primitives",
    "//apps/fwd:onos-apps-fwd-native",
    "//protocols/openflow/api:onos-protocols-openflow-api-native",
    "//protocols/openflow/api:onos-protocols-openflow-api-tests",
    "//protocols/openflow/ctl:onos-protocols-openflow-ctl-native",
    "//protocols/openflow/ctl:onos-protocols-openflow-ctl-tests",
]

osgi_jar_with_tests(
    karaf_command_packages = ["org.onosproject.eHDDP"],
    deps = COMPILE_DEPS,
)

onos_app(
    app_name = "org.onosproject.eHDDP",
    category = "Provider",
    description = "Protocol eHDDP",
    required_apps = [
        "org.onosproject.openflow-base",
        "org.onosproject.hostprovider",
        "org.onosproject.linkdiscovery",
        #"org.onosproject.netcfglinksprovider",
    ],
    title = "eHDDP",
    url = "http://onosproject.org",
)
