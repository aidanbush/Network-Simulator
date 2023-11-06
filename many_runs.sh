#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]' '[dest_id]' '[dest_id,deflect_probability]' '[dest_id,drop_probability]')
#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[1-2_hop_shortest]')
#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]')
#states=('[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section]')

#states=('[1_hop_shortest,3x3_section]')
states=('[1_hop_shortest,3x3_section]_prop_0.01' '[1_hop_shortest,3x3_section]_prop_0.5')

#states=('[1_hop_shortest]' '[1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[1-2_hop_shortest]' '[1-2_hop_shortest]' '[1-2_hop_shortest,3x3_section]')

#states=('[1_hop_shortest,3x3_section]' '[1_hop_shortest,3x3_section,deflect_probability]' '[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section]' '[2_hop_shortest,1_hop_shortest,3x3_section,deflect_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]')
#states=('[dest_id]')
#states=('[2_hop_shortest,1_hop_shortest,3x3_section]' '[dest_id]')
#states=('[1_hop_shortest,3x3_section,drop_probability]' '[2_hop_shortest,1_hop_shortest,3x3_section,drop_probability]' '[dest_id,drop_probability]')
states=()
utils=(05 1 15 2)

#agents=('rand_deflect' 'rand_forward')
#agents=('rand_forward_prop_0.5' 'rand_forward_prop_0.01')
agents=('rand_forward')

for state in ${states[@]} ; do
    #agents+=('mbd_original_'${state})
    agents+=('mbd_slide_'${state})
    #agents+=('mbd_D-LinUCB_0.9_'${state})
    #agents+=('mbd_D-LinUCB_0.99_'${state})
    #agents+=('mbd_D-LinUCB_0.999_'${state})
    #agents+=('mbd_D-LinUCB_0.9999_'${state})
    #agents+=('mbd_D-LinUCB_0.99999_'${state})
done

n=8
network_size="${n}x${n}"
parallel=30

for util in ${utils[@]}; do
    for agent in ${agents[@]} ; do
        #18000
        bash run_test.sh -t 50000 -p $parallel -n $parallel configs/${network_size}/${agent}/fc_200/u_0.${util} ${network_size}_bursty_0.${util}_${agent}_fc_200 ${network_size}manhattan_flow_params.json
        #bash run_test.sh -t 50000 -p $parallel -n $parallel configs/${network_size}/${agent}/fc_200/u_0.${util} delete ${network_size}manhattan_flow_params.json
    done
done
